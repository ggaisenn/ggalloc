#include <cstddef>


//Defining the structure of our metadata 
struct meta{

    size_t size; //to check size of memory block requested by the user, and to handle it among various OS versions(ie. bits)
    int free;  //1=free, 0=used
    int magic; //to check corruption/Integrity of block
    struct meta* next; //next block
};

/*To ensure that the metdata structure is properly aligned in memory, we can align the memory blocks to 16 bytes
for 64-bit systems -> 16 bytes
for 32-bit systems -> 8 bytes
for 8-bit/16-bit systems -> 4 bytes*/
#define ALIGNMENT 16

/*A bitwise operation to round up to the nearest multiple of alignment
eg: ALIGN([1,16]) = 16, ALIGN([17,32]) = 32 ..ALIGN([241,256]) = 256 and so on*/
#define ALIGN(size) ((size + (ALIGNMENT-1))& ~(ALIGNMENT-1))


#define SIZEOFMETA sizeof(struct meta) //used to manage the metadata of each block
#define MAGIC 0xDEADBEEF 
/*if a user accidentally writes past their allocated memory(leading to an overflow), this field will be overridden first, 
providing us with a heap corruption alert,thereby enabling us to handle the error which would have otherwise gone unnoticed, 
leading to a crash in the program*/



/*
As the OS will proivde us with a large chunk of raw memory, we need to manage it in a way that will allow us to allocate and free memory efficiently
We need to use precise pointer arithemetic that will enable us to find whre the tracking info and the user data

This is how our block of memory will look like, 
 Block = [Metadata|Payload]
 Metadata gives us information about the block. It holds our size, free status, magic number, and a pointer to the next block.
 Payload contains the actual data that is allocated by the user. User gets to read from and write to this zone
 You can consider Metadata as a Label of the block, 
 and Payload as the actual content
 
 Our allocator will treat this block as a single unit of memory, 
 but when a user interacts with this allocator, they will only see the payload zone in that block.

 Therefore, our heap will consist of a chain of such blocks
 
 [META|PAYLOAD] -> [META|PAYLOAD] -> [META|PAYLOAD] ->...... ->[META|PAYLOAD] -> NULL
   BLOCK 1.           BLOCK 2.         BLOCK 3.                   BLOCK N    
   
Our allocator will manage these blocks. 
User asks memory -> One Block gets carved. User frees the memory -> Block gets freed
*/

/* Pointer Scaling => if we have a metadata pointer, doing pointer+1 jumps completely past this structure to the next structure of block meta type.

As we have defined the size of our structure with regards to the alignment padding(ie. 32, 48, 64 ...), we can break pointer scaling by casting our pointer to a char pointer as is exactly 1 byte,
therefore when we do (char*)ptr + META_SIZE, we can. move the pointer by exaclty META_SIZE number of bytes*/


#define PAYLOAD(block) ((void*)((char*)(block) + SIZEOFMETA))
/* This macro is used to get the payload pointer from the block pointer,
  assume the size of our block to be 0x2000, then we would typecast our block pointer to a char such that each pointer arithemetic steps exactly 1 byte per integer added
  then we align the pointer to the start of the payload by adding SIZEOFMETA to it => 0x2000 + 32 => 0x2000 + 0x20 => 0x2020
  we would typecast the pointer to void to make it generic and hand the pointer 0x2020 back to the user such that the user can enter data of any type.
The user can now safely write data to 0x2020 without corrupting the structure hidden behind them at 0x2000*/


#define META(block) ((struct meta*)((char*)(block) - SIZEOFMETA))
/* This macro is used to get the metadata pointer from the payload pointer,
  assume the size of our block to be 0x2020, then we would the same typecast operation as we did in GET_PAYLOAD and subtract SIZEOFMETA from the payload pointer
   to get back to the start of the block => 0x2020 - 32 => 0x2020 - 0x20 => 0x2000
we would typecast the pointer to struct meta* which will be used by the allocator to free the block if needed, read its size, or check for block integrity.*/


// To get payload => look forward
// To get metadata => look backward
