#include "../allocator.cpp"
#include <iostream>
#include <cstdio>
int main() {
    void* a = ggalloc(32);
    void* b = ggalloc(32);
    void* c = ggalloc(32);
    ggfree(a); ggfree(b); ggfree(c);  
    
    int n = 0;
    for (struct meta* m = (struct meta*)global; m; m = m->next) {
        cout << "Block " << n++ << ": size=" << m->size << ", free=" << m->free;
    }
    cout << endl;;

    void* d = ggalloc(64);
    cout << "alloc after coalesce: " << (d ? "OK" : "FAILED") << endl;
    return 0;
}