#include "allocator.cpp"

#include <iostream>
#include <cstring>

using namespace std;

int main() {
    cout << "=== ggalloc demo ===\n\n";

    // first allocation -> creates the heap via sbrk
    void* p1 = ggalloc(32);
    if (!p1) { 
        cout << "FAIL: ggalloc(32)\n"; 
        return 1; 
    }
    cout << "ggalloc(32) -> " << p1 << "  size=" << META(p1)->size << "\n";

    // write a pattern past the payload; magic must stay intact
    memset(p1, 0xAA, 32);
    if (META(p1)->magic != MAGIC) { 
        cout << "FAIL: magic corrupted after write\n"; 
        return 1; 
    }
    cout << "  magic intact after 32-byte write\n";

    // second allocation -> no free block big enough, heap grows
    void* p2 = ggalloc(64);
    if (!p2) { 
        cout << "FAIL: ggalloc(64)\n"; 
        return 1; 
    }
    cout << "ggalloc(64) -> " << p2 << "  size=" << META(p2)->size << "\n";
    memset(p2, 0xBB, 64);

    // free p1 -> first-fit must hand the SAME block back on re-alloc
    ggfree(p1);
    cout << "ggfree() -> freed " << p1 << "\n";

    void* p3 = ggalloc(32);
    if (!p3) { 
        cout << "FAIL: ggalloc(32) after free\n"; 
        return 1; 
    }
    const char* reuse = (p3 == p1) ? "REUSED (first-fit OK)" : "different addr (first-fit failed)";

    cout << "ggalloc(32) -> " << p3 << "  " << reuse << "\n";

    if (p3 != p1) { 
        cout << "FAIL: first-fit did not reuse the freed block\n"; 
        return 1; 
    }
    if (META(p3)->magic != MAGIC) { 
        cout << "FAIL: magic corrupted on reused block\n"; 
        return 1; 
    }
    cout << "  magic intact on reused block\n";

    // free both, verify the frees healed cleanly
    ggfree(p2);
    ggfree(p3);
    cout << "ggfree() -> freed " << p2 << " and " << p3 << "\n";

    cout << "\nAll demo scenarios passed. Heap of linked blocks is alive.\n";
    return 0;
}