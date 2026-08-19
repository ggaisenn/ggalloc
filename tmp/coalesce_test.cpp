#include "allocator.cpp"
#include <iostream>
#include <cstdio>
using namespace std;

int main() {
    void* a = ggalloc(32);
    void* b = ggalloc(32);
    void* c = ggalloc(32);
    ggfree(a); ggfree(b); ggfree(c);  
    
    int n = 0;
    for (struct meta* m = (struct meta*)global; m; m = m->next) {
        cout << "Block " << n++ << ": size=" << m->size << ", free=" << m->free;
    }
    cout << endl;

    // Day 1 verification: three merged 32-byte blocks = ONE block,
    // 160 bytes of payload, free (32 + 32 + 32 + 32 + 32).
    if (n != 1) {
        cout << "FAIL: expected 1 block after coalesce, found " << n << "\n";
        return 1;
    }
    struct meta* merged = (struct meta*)global;
    if (merged->size != 160 || merged->free != 1) {
        cout << "FAIL: expected one free block of size 160, got size="
             << merged->size << " free=" << merged->free << "\n";
        return 1;
    }

    void* d = ggalloc(64);
    cout << "alloc after coalesce: " << (d ? "OK" : "FAILED") << endl;
    if (!d) return 1;
    return 0;
}