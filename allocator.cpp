#include "allocator.h"
#include <iostream>
#include <unistd.h> //POSIX header
#include <cassert>
using namespace std;

//The absolute start of our heap
//When the program launches we have 0 memory allocated
void* base = nullptr; //When we want to search our heap, we will start from here

//First-Fit Search Algorithm
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

struct meta* spacerequest(struct meta* ptr, size_t size){

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




int main(){

}
