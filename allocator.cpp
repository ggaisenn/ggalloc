#include "allocator.h"
#include <iostream>
#include <unistd.h> //POSIX header
#include <cassert>
using namespace std;

//The absolute start of our heap
//When the program launches we have 0 memory allocated
void* global = nullptr; //When we want to search our heap, we will start from here

//Fist-Fit Search Algorithm
/*To find our desired block, we have to make sure that 
 1) its free
 2) it has sufficient size
*/
struct meta* free_block(struct meta** ptr, size_t size){

    //Start traversing from the beginning of the heap
    struct meta* curr = (struct meta*)global;

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


    char* byte = (char*)block; //We''l use char pointer arithmetic to step byte-by-byte
    struct meta* new_b = (struct meta*)(byte + SIZEOFMETA + size);
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

//The Core Allocator
void* ggalloc(size_t size){

    //If user reuqests for 0 bytes, we will return null
    if(size <=0){
        return nullptr;
    }

    //Variable used to obtain refined alignment
    size_t size_aligned = ALIGN(size);

    struct meta* blk = nullptr;

    /*if our global is null, we'll skip searching and request space to get our initial chunk of RAM 
    if we already have a heap, we'll find a free block*/

    if(global==nullptr){
        blk = space_request(nullptr, size_aligned);
        if(!blk) return nullptr; //Our request for memory denied by the OS
        global = blk;
    }else{
        //We'll search the heap, to find a free block
        struct meta* ptr = (struct meta*)global; //We'll keep track of the previous block in our search to attach new blocks if needed                 
        blk = free_block(&ptr, size_aligned);
        if(blk){

            //Check if we can split the block to save space
            if((blk->size - size_aligned)>=(SIZEOFMETA + ALIGNMENT)){
                block_split(blk, size_aligned);

            }else{
                /* We don't need to split the block, 
                We'll give the whole block */
                blk->free=0;
                blk->magic = MAGIC;
            }

        }else{
            /*If our search still turns up empty, 
            we'll pass the block to request space 
            so that we can attach a new chunk of memory to the end of our list*/
            blk = space_request(ptr, size_aligned);
            if(!blk) return nullptr;
        }
    }

    //Return the payload address to the user
    return PAYLOAD(blk);

}

//To verify if a random memory is inside our heap
bool valid_heap(void* ptr){

    if(!global || !ptr) return false;

    //sbrk(0) gives the highest address of our current heap
    void* heap_top = sbrk(0);

    //The pointer must fall between the heap start and the end of the heap
    return (ptr>=global && ptr < heap_top);
}

/*-----------------GARBAGE-COLLECTOR-----------------
1)MARK PHASE
2)SWEEP PHASE
To prevent memory leaks, dangling pointers, and other issues related 
to manual memory management, we can implement a simple mark-and-sweep GC
*/

/*
1)MARK PHASE => Scans the Program "Root Set"(includes local and static/global varaiables) to identify all memory blocks that are reachable by the user
If the block is reachable, we'll mark it as safe
If the block is not reachable, we'll consider it as garbage
*/

void mark_phase(void** stack_b, void** stack_t){
    void** start = stack_b;
    void** end = stack_t;

    /*The stack goes downwards, 
    so we swap them to iterate safely from lowest to highest address.*/
    if (start>end){
        start= stack_t;
        end = stack_b;
    }

    
    //We'll scan through the stack
    for(void** p = start; p<end; p++){
        void* potential = *p; //To get the potential address

        if(valid_heap(potential)){

            //Step backwards to find the metadata
            struct meta* block = META(potential);

            //If the block is valid and used, we'll mark it as reachable
            if(block->magic == MAGIC && block->free==0){
                block->is_reachable = 1; //The block survives garbage collection
            }

        }

    }
    
}


/*2)SWEEP PHASE => Iterates through the heap to reclaim any blocks that are not marked as reachable and return them to the free list
Any memory the user lost track of is recycled automatically to prevent any leaks*/

void sweep_phase(){


    struct meta* curr = (struct meta*)global;

    //We'll check through every single block we've allocated
    while(curr!=nullptr){

        //We only care about the blocks that are currently in use
        if(curr->free==0){
            //If the block is not reachable, the user lost track of it, so we reclaim the block by marking it as free
            if(curr->is_reachable==0){
                curr->free = 1;

            }else{
                //The block is reachable, but we need to reset the mark so that it can be evaluated for the next GC Cycle
                curr->is_reachable = 0;
            }

        }

        //Move to the next block in our linked list
        curr = curr->next;
    }
}






int main(){

    
}
