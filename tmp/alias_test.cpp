#include "allocator.h"
#include <iostream>
#include <cstdio>
using namespace std;

int main() {
    char* p1 = (char*)ggalloc(200);
    ggfree(p1);
    char* p2 = (char*)ggalloc(64);
    char* p3 = (char*)ggalloc(32);
    cout << "p2=" << (void*)p2 << " p3=" << (void*)p3 << " " << (p2 == p3 ? "ALIASED!" : "distinct") << endl;
    // two LIVE allocations must never share an address; exit code = result
    return (p2 == p3) ? 1 : 0;
}
