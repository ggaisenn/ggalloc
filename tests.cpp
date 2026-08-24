#include "allocator.h"

#include <cassert>
#include <iostream>
#include <cstring>
#include <sys/wait.h>
#include <csignal>

// sbrk is deprecated on macOS; needed only to measure heap growth in the
// coalesce test. Same suppression as allocator.cpp.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#include <unistd.h>
static void* heap_top() { return sbrk(0); }
#pragma clang diagnostic pop

using namespace std;

// The heap head lives in allocator.cpp. Not public API — but tests are
// allowed to peek at internals.
extern void* global;

// count used blocks by walking the free list
static int count_used() {
    int n = 0;
    for (meta* m = (meta*)global; m; m = m->next)
        if (!m->free) ++n;
    return n;
}

// ALIGN macro boundaries: round up to 16, with 0 -> 0
static void test_align() {
    assert(ALIGN(0) == 0);
    assert(ALIGN(1) == 16);
    assert(ALIGN(15) == 16);
    assert(ALIGN(16) == 16);
    assert(ALIGN(17) == 32);
    assert(ALIGN(241) == 256);
}

// block_split: a returned 128-byte block must be split
// on a 16-byte request, leaving a free remainder, and first-fit must reuse
// the same address
static void test_split() {
    void* p = ggalloc(128);
    assert(p && "ggalloc(128) failed");
    memset(p, 0xAB, 128);
    assert(META(p)->magic == MAGIC && "magic corrupted by 128-byte write");

    ggfree(p);

    void* q = ggalloc(16);
    assert(q && "ggalloc(16) after free failed");
    assert(q == p && "first-fit did not reuse the freed block");

    assert(META(q)->size == 16 && "block was not resized by split");
    meta* rem = META(q)->next;
    assert(rem && "split produced no remainder block");
    assert(rem->free == 1 && "remainder is not free");
    assert(rem->size == 128 - 16 - SIZEOFMETA && "remainder size wrong");

    memset(q, 0xCD, 16);
    assert(META(q)->magic == MAGIC && "magic corrupted by 16-byte write");
    ggfree(q);
}

// coalesce: three freed 32-byte blocks merge into a
// region big enough for a 96-byte request WITHOUT growing the heap
static void test_coalesce() {
    void* a = ggalloc(32);
    void* b = ggalloc(32);
    void* c = ggalloc(32);
    assert(a && b && c && "three ggalloc(32) failed");
    memset(a, 0x11, 32);
    memset(b, 0x22, 32);
    memset(c, 0x33, 32);
    assert(META(a)->magic == MAGIC && META(b)->magic == MAGIC &&
           META(c)->magic == MAGIC && "magic corrupted");

    void* top_before = heap_top();
    ggfree(a);
    ggfree(b);
    ggfree(c);

    void* d = ggalloc(96);
    assert(d && "96-byte request failed after frees (coalesce did not merge)");
    assert(heap_top() == top_before &&
           "heap grew: 96-byte request was NOT satisfied by coalesced region");
    memset(d, 0x44, 96);
    assert(META(d)->magic == MAGIC && "magic corrupted on coalesced block");

    ggfree(d);
}

// valid_heap: bounds check against [global, sbrk(0))
static void test_valid_heap() {
    void* p = ggalloc(32);
    assert(p && "ggalloc(32) failed");

    assert(valid_heap(p) && "real payload not recognized");
    assert(valid_heap(META(p)) && "block header not recognized");
    assert(!valid_heap(nullptr) && "nullptr reported in-heap");
    assert(!valid_heap(reinterpret_cast<void*>(0x1)) && "garbage address reported in-heap");
    assert(!valid_heap(reinterpret_cast<void*>(-1)) && "huge address reported in-heap");
    assert(!valid_heap(heap_top()) && "heap top must be exclusive");

    ggfree(p);
    assert(valid_heap(p) &&
           "freed payload must stay in heap range (bounds check is address-only)");
}

// gggc: blocks whose pointers were dropped must be reclaimed; blocks
// still referenced must survive the cycle.
static void test_gc() {
    void* keep = ggalloc(32);
    assert(keep && "ggalloc(32) failed");
    memset(keep, 0xEE, 32);  // 0xEE... is pointer-shaped but far outside
                             // the heap range -> can't fake a reference

    void* drop1 = ggalloc(48);
    void* drop2 = ggalloc(48);
    assert(drop1 && drop2 && "ggalloc(48) x2 failed");
    memset(drop1, 0xF1, 48);
    memset(drop2, 0xF2, 48);

    assert(count_used() == 3 && "heap not in expected state (keep+2 drops)");

    // THE DROP: overwrite the only live references. Any slot a stack
    // scan can still see must NOT hold the old addresses.
    drop1 = nullptr;
    drop2 = nullptr;
    asm volatile("" ::: "memory");  // compiler barrier: the null stores
                                    // above must happen in real memory

    gggc();  // one full mark-and-sweep cycle

    // `keep` must survive: used, uncorrupted, and its mark reset for the
    // next cycle (the Day-1 reset invariant, now exercised for real).
    assert(META(keep)->free == 0 && "live block was swept by gggc");
    assert(META(keep)->magic == MAGIC && "live block corrupted by gggc");
    assert(META(keep)->is_reachable == 0 &&
           "reachable mark was not reset after sweep");

    assert(count_used() == 1 && "garbage blocks were NOT reclaimed by gggc");

    // The block physically after `keep` was drop1; after the cycle it
    // must head the coalesced 128-byte free region (48 + header + 48).
    meta* after_keep = (meta*)((char*)META(keep) + SIZEOFMETA + 32);
    assert(after_keep->free == 1 &&
           "block after keep was not reclaimed in place");
    assert(after_keep->size >= 48 + SIZEOFMETA + 48 &&
           "reclaimed region smaller than 48 + header + 48");

    ggfree(keep);
}

// ggfree must REJECT freeing an already-freed block (glibc aborts too).
// The abort kills the process, so probe it in a child: the parent checks
// the child died with SIGABRT.
static void test_double_free() {
    void* p = ggalloc(32);
    assert(p && "ggalloc(32) failed");

    ggfree(p);  // first free: legal

    pid_t pid = fork();
    assert(pid >= 0 && "fork failed");
    if (pid == 0) {
        ggfree(p);   // second free: must abort
        _exit(0);    // unreachable if the guard works
    }
    int status = 0;
    waitpid(pid, &status, 0);
    assert(WIFSIGNALED(status) && WTERMSIG(status) == SIGABRT &&
           "double free was NOT detected (child did not abort)");
}

// ggfree must REJECT pointers that are not inside our heap (a stack
// variable here) instead of walking backward into foreign memory.
static void test_invalid_free() {
    int stack_var = 42;   // definitely not a heap pointer

    pid_t pid = fork();
    assert(pid >= 0 && "fork failed");
    if (pid == 0) {
        ggfree(&stack_var);  // must abort (outside [global, sbrk(0)))
        _exit(0);
    }
    int status = 0;
    waitpid(pid, &status, 0);
    assert(WIFSIGNALED(status) && WTERMSIG(status) == SIGABRT &&
           "invalid-pointer free was NOT detected (child did not abort)");
}

int main() {
    int passed = 0;
    const int total = 7;

    test_align();
    ++passed;
    cout << "test_align: PASS\n";
    
    test_split();
    ++passed;
    cout << "test_split:  PASS\n";

    test_coalesce();
    ++passed;
    cout << "test_coalesce: PASS\n";

    test_valid_heap();
    ++passed;
    cout << "test_valid_heap: PASS\n";

    test_gc();
    ++passed;
    cout << "test_gc:         PASS\n";

    test_double_free();
    ++passed;
    cout << "test_double_free:  PASS\n";

    test_invalid_free();
    ++passed;
    cout << "test_invalid_free: PASS\n";

    cout << "\n" << passed << "/" << total << " tests passed\n";
    return 0;
}