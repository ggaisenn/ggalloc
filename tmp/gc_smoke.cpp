#include "allocator.h"
int main(){
    void* a = ggalloc(32);
    void* b = ggalloc(64);
    gggc();                 // a and b are on the stack -> must survive
    gggc();                 // twice, with the heap in a steady state
    ggfree(a);
    ggfree(b);
}