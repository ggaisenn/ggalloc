#include "../allocator.cpp"
#include <iostream>
#include <cstdio>

int main() {
for (int i = 0; i < 100; i++) { void* p = ggalloc(64); ggfree(p); }
void* big = ggalloc(100);         
}