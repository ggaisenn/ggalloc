#include <cstddef>

struct block_meta{

    size_t size; //to check block size, and to handle it among various OS versions(ie. bits)
    int free;  //1=free, 0=used
    int check; //to check corruption
    struct block_meta* next; //next block
};


#define META_SIZE sizeof(struct block_meta)
#define MAGIC_NUM 0xDEADBEEF