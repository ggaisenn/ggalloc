#include "allocator.h"
#include <iostream>

extern void* global;

static int count_used() {
    int n = 0;
    for (meta* m = (meta*)global; m; m = m->next)
        if (!m->free) ++n;
    return n;
}

int main() {
    for (int i = 0; i < 100; i++) { void* p = ggalloc(64); ggfree(p); p = nullptr; }

    void* keep = ggalloc(32);
    std::cout << "used blocks before gggc: " << count_used() << "\n";

    keep = nullptr;
    asm volatile("" ::: "memory");

    gggc();

    int after = count_used();
    std::cout << "used blocks after gggc:  " << after << "\n";
    return after == 0 ? 0 : 1;  
}