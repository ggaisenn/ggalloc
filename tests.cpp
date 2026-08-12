#include "allocator.h"

#include <cassert>
#include <iostream>
#include <cstring>

// sbrk is deprecated on macOS; needed only to measure heap growth in the
// coalesce test. Same suppression as allocator.cpp.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#include <unistd.h>
static void* heap_top() { return sbrk(0); }
#pragma clang diagnostic pop

using namespace std;

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

int main() {
    int passed = 0;
    const int total = 4;

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

    cout << "\n" << passed << "/" << total << " tests passed\n";
    return 0;
}