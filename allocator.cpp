#include "allocator.h"
#include <iostream>
#include <unistd.h> //POSIX header
#include <cassert>
using namespace std;

//The absolute start of our heap
//When the program launches we have 0 memory allocated
void* base = nullptr; //When we want to search our heap, we will start from here

//Fist-Fit Search Algorithm
/*To find our desired block, we have to make sure that 
 1) its free
 2) it has sufficient size
*/
struct meta* free_block(struct meta** ptr, size_t size){

    //Start traversing from the beginning of the heap
    struct meta* curr = (struct meta*)base;

    while(curr && !(curr->free && curr->size >= size)){
        //Moving to the next block, whilst keeping track of the previous block
        *ptr = curr;
        curr = curr->next;
    }

    return curr;
    //If a block is found, curr will point to it
    /*
     If we couldn't find one(ie. reached the end), curr will be a nullptr. We would need to reques the OS for a brand new memory.
     When we receiver that memory we would need to attached it to the end of our linked list. As we keep on updating *ptr to curr, we can ensure that even if our search fails, the ptr will hold the address of the very last block in our list. 
     Thus, this would enable us to attach a new block later to the block pointed to by ptr.
    */
    

}

/* To request for fresh memory from the RAM, we will interface directly with the OS kernel.
 We will use the sbrk() system call (belongs to the POSIX standard) for this purpose.
This function is used to move the program break(ie the boundary of the heap) upwards, thus providing us with new encompassed space */

struct meta* space_request(struct meta* ptr, size_t size){

    /*The OS doesn't care about our metadata, 
    thus we'll need to calculate the total memory with regards to 
    the metadata size as well as the size of our header*/

    size_t total = size + SIZEOFMETA;
    //           payload size + metadata size

    struct meta* block = (meta*)sbrk(0); //sbrk(0) gives us the current program break of the heap, this address will be the starting point of our new block

    //We'll request the OS to expand the heap by 'total' bytes, sbrk(total) pushes the heap boundary upwards
    void* request = sbrk(total);

    //If the sbrk fails to allocate memory, it will return -1 casted as a void pointer
    if(request == (void*)-1){
        return nullptr; //out of memory, we'll safely fail the allocation
    }

    /*If the block is not the first one we've allocated, link the old end of the block 
    to the brand new block to expand the heap*/
    if(ptr!=nullptr){
        ptr->next = block;
    }

    //We'll initialize our header for this new block
    block->size = size;  
    block->free = 0;         //Block in use
    block->magic = MAGIC;    //Set our corruption alert
    block->next = nullptr;  //The new end of the list

    return block;

}

/* Suppose if a user had previously allocated a massive block of memory and then freed it. Later on they want to use a block of memory that is smaller than the one they freed.
Our First-Fit search algorithm will hand over the previous block without checking the size of the new block requested by the user, leading to a wastage of heap space. 
This phenomenon is referred to as Internal Fragmentation.
So we'll make a function that will split this block to the size requested by the user and the remainder of the space in our free list for future use*/

void block_split(struct meta* block, size_t size){


    char* base = (char*)block; //We''l use char pointer arithmetic to step byte-by-byte
    struct meta* new_b = (struct meta*)(base + SIZEOFMETA + size);
    //                            Start + Size of metadata + Payload Size

    //Initialize the remainder space as a new block in our free list
    new_b->size = block->size - size - SIZEOFMETA; 
    new_b->free = 1; // The remainder block is free
    new_b->magic = MAGIC; 
    new_b->next = block->next; //The new block will point to the block pointed by the previous one

    //Adjusting the original block to fit the block size requested by the user
    block->size = size;
    block->next = new_b; //Point to our new block

}




int main(){

    
}
