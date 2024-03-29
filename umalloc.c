#include "umalloc.h"
#include "csbrk.h"
#include "ansicolors.h"
#include <stdio.h>
#include <assert.h>

const char author[] = ANSI_BOLD ANSI_COLOR_RED "Tuan Pham, tqp85" ANSI_RESET;
intptr_t csbrk_call = (PAGESIZE * 16);
/*
 * The following helpers can be used to interact with the memory_block_t
 * struct, they can be adjusted as necessary.
 */

// A sample pointer to the start of the free list.
memory_block_t *free_head;

/*
 * is_allocated - returns true if a block is marked as allocated.
 */
bool is_allocated(memory_block_t *block) {
    assert(block != NULL);
    return block->block_size_alloc & 0x1;
}

/*
 * allocate - marks a block as allocated.
 */
void allocate(memory_block_t *block) {
    assert(block != NULL);
    block->block_size_alloc |= 0x1;
}


/*
 * deallocate - marks a block as unallocated.
 */
void deallocate(memory_block_t *block) {
    assert(block != NULL);
    block->block_size_alloc &= ~0x1;
}

/*
 * get_size - gets the size of the block.
 */
size_t get_size(memory_block_t *block) {
    assert(block != NULL);
    //return block->block_size_alloc & ~(ALIGNMENT-1);
    if (is_allocated(block)){
        return block->block_size_alloc - 1;
    }
    return block->block_size_alloc;
}

/*
 * The following are helper functions that can be implemented to assist in your
 * design, but they are not required. 
 */

/*
 * find - finds a free block that can satisfy the umalloc request.
 * return the fitted block after splitting the correct node
 * return the last node in the list if we can't find anything
 */
memory_block_t *find(size_t size) {
    // using first and best fit for now
    memory_block_t *prev_node = NULL;
    memory_block_t *cur_node = free_head;
    while (cur_node != NULL){
        if (cur_node->block_size_alloc >= size){
            size_t old_size = cur_node->block_size_alloc;
            memory_block_t *cur_node_next = cur_node->next;
            // split to two new parts
            memory_block_t *ret = split(cur_node, size);
            memory_block_t *new_node = split_new(ret, cur_node, 
                                            old_size, cur_node_next);
            if(prev_node != NULL){
                prev_node->next = new_node;
            }
            if(prev_node == NULL){
                free_head = new_node; // first one got change
            }
            return ret;
        }
        prev_node = cur_node;
        cur_node = cur_node->next;
    }
    // never found one return the last one in list
    return prev_node;
}

/*
 * split - splits a given block in parts, one allocated, one free.
 */
memory_block_t *split(memory_block_t *block, size_t size) {
    memory_block_t *ret;
    int remainder = (uintptr_t) block % 16;
    if (remainder == 0){
        ret = block;
    } else {
        ret = (memory_block_t *) ((uintptr_t) block - remainder + ALIGNMENT);
    }
    ret->block_size_alloc = size - sizeof(memory_block_t);
    ret->next = (memory_block_t *) ((char *) (ret + 1) + ret->block_size_alloc);
    allocate(ret);
    return ret;
}

/*
 * split - return the left over portion back
 */
memory_block_t *split_new(memory_block_t *taken, memory_block_t *space, 
                                    size_t old_size,  memory_block_t *old_next) {
    int padding = (uintptr_t) taken - (uintptr_t) space;
    memory_block_t *ret = (memory_block_t *) ((char *) space + padding + 
                sizeof(memory_block_t) + get_size(taken));
    ret->block_size_alloc = old_size - get_size(taken) - sizeof(memory_block_t) - padding;
    ret->next = old_next;
    return ret;
}

/*
 * check if need to coalese between two nodes (lead and trailer)
 * this take in two inputs and combine the nodes
 */
void coalesce(memory_block_t *lead, memory_block_t *trailer, 
                    size_t lead_end, size_t trailer_begin){
    size_t padding = trailer_begin - lead_end; // bc trailer is higher address
    assert(padding >= 0); 
    // if the padding is <= alignment then we coalesce 
    if (padding <= ALIGNMENT){
        // lead pointer is now pointer the new entire free block
        lead->block_size_alloc = padding + sizeof(memory_block_t) +
            lead->block_size_alloc + trailer->block_size_alloc;
        lead->next = trailer->next; // connect to trailer next
        return;
    } 
    lead->next = trailer;
    return;
}

/*
* linear search to find the right place and add it
* return the very left node
*/
void add_to_heap(memory_block_t *block){
    memory_block_t *cur_node = free_head;
    // case 1 (block is the very first node)
    if (cur_node > block){
        coalesce(block, cur_node, 
                (size_t) block->next, (size_t) cur_node);
        free_head = block; // now new block is free_head
        return;
    }
    
    while (cur_node != NULL){
        memory_block_t *next_node = cur_node->next;
        size_t cur_node_end = (uintptr_t)(cur_node + 1) + cur_node->block_size_alloc;
        // case 2 (block is the very last node)
        if (next_node == NULL && cur_node < block){
            coalesce(cur_node, block, (size_t) cur_node_end, (size_t) block);
            block->next = NULL;
            return;  
        }
        // case 3 (block is between cur and next node)
        if (cur_node < block && next_node > block){
            // check 2nd and 3rd node
            coalesce(block, next_node, (size_t) block->next, (size_t) next_node);
            // check 1st and 2nd node
            coalesce(cur_node, block, (size_t) cur_node_end, (size_t) block);
            return;
        }
        cur_node = next_node;
    }
    assert(cur_node == NULL); //something is wrong
} 

/*
 * print list for debugging by going over free_head
 */
void print_heap() {
    memory_block_t *cur_node = free_head;
    while(cur_node != NULL){
        printf("%s at %p, size %d, next address %p\n",
                (is_allocated(cur_node))?"alllocated":"free",
                cur_node,
                (int) (cur_node->block_size_alloc), cur_node->next);
        cur_node = cur_node->next;       
    }
}

/*
 * uinit - Used initialize metadata required to manage the heap
 * along with allocating initial memory.
 */
int uinit() {
    free_head = csbrk(csbrk_call); //intialize free_head to the max increment
    // set size to max at first
    free_head->block_size_alloc = csbrk_call - sizeof(memory_block_t);
    free_head->next = NULL; // next is pointing to NULL
    return 0;
}

/*
 * umalloc -  allocates size bytes and returns a pointer to the allocated memory.
 */
void *umalloc(size_t size) {
    int mod = size % ALIGNMENT;
    if (mod != 0)
        size += ALIGNMENT - mod; 
    size += sizeof(memory_block_t); // size we're finding must be % 16
    memory_block_t *fitted_block = find(size);
    if (fitted_block->next == NULL){ // didn't find a block with enough space
        memory_block_t *last_node = fitted_block;
        intptr_t new_size = size * 15;
        if (new_size >= PAGESIZE * 16) // if call size is too big
            new_size = PAGESIZE * 16;
        memory_block_t *more_space = csbrk(new_size);
        more_space->block_size_alloc = new_size - sizeof(memory_block_t);
        more_space->next = NULL; 
        fitted_block = split(more_space, size);
        memory_block_t *left_over = split_new(fitted_block, more_space, 
                                new_size - sizeof(memory_block_t), NULL);
        last_node->next = left_over;
    }
    return fitted_block + 1;
}

/*
 * ufree -  frees the memory space pointed to by ptr, which must have been called
 * by a previous call to malloc.
 */
void ufree(void *ptr) {
    // go back to the header
    memory_block_t *cur_node = ((memory_block_t *) ptr) - 1;
    deallocate(cur_node);
    add_to_heap(cur_node);;
}