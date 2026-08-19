#include "allocator.h"
#include <cassert>
#include <iostream>

// The heap head lives in allocator.cpp — tests peek at internals.
extern void* global;

static int count_used() {
    int n = 0;
    for (meta* m = (meta*)global; m; m = m->next)
        if (!m->free) ++n;
    return n;
}

int main() {
    // dirty the heap first: fresh sbrk pages are zeroed, which hides
    // uninitialized-field bugs — recycled payload bytes don't lie.
    // NOTE: first-fit hands back the SAME block every iteration, so p
    // is nulled each round — otherwise a stale copy of that address on
    // the stack would make the collector (correctly!) retain the block.
    for (int i = 0; i < 100; i++) { void* p = ggalloc(64); ggfree(p); p = nullptr; }

    void* keep = ggalloc(32);
    assert(keep && "ggalloc(32) failed");
    std::cout << "used blocks before gggc: " << count_used() << "\n";

    // drop the ONLY reference: zero the live slot, and force the store
    // to really happen (compiler barrier — the compiler may legally
    // elide a store to a local whose address never escaped)
    keep = nullptr;
    asm volatile("" ::: "memory");

    gggc();

    int after = count_used();
    std::cout << "used blocks after gggc:  " << after << "\n";
    return after == 0 ? 0 : 1;   // exit 1 = collector failed to reclaim
}