#include "umalloc.h"
#include "csbrk.h"
#include "ansicolors.h"
#include <stdio.h>
#include <assert.h>

const char author[] = ANSI_BOLD ANSI_COLOR_RED "Tuan Pham, tqp85" ANSI_RESET;

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
 * get_next - gets the next block.
 */
memory_block_t *get_next(memory_block_t *block) {
    assert(block != NULL);
    return block->next;
}

/*
 * put_block - puts a block struct into memory at the specified address.
 * Initializes the size and allocated fields, along with NUlling out the next 
 * field.
 */
void put_block(memory_block_t *block, size_t size, bool alloc) {
    assert(block != NULL);
    assert(size % ALIGNMENT == 0);
    assert(alloc >> 1 == 0);
    block->block_size_alloc = size | alloc;
    block->next = NULL;
}

/*
 * get_payload - gets the payload of the block.
 */
void *get_payload(memory_block_t *block) {
    assert(block != NULL);
    return (void*)(block + 1);
}

/*
 * get_block - given a payload, returns the block.
 */
memory_block_t *get_block(void *payload) {
    assert(payload != NULL);
    return ((memory_block_t *)payload) - 1;
}

/*
 * The following are helper functions that can be implemented to assist in your
 * design, but they are not required. 
 */

/*
 * find - finds a free block that can satisfy the umalloc request.
 */
memory_block_t *find(size_t size) {
    // using first and best fit for now
    memory_block_t *prev_node = NULL;
    memory_block_t *cur_node = free_head;
    while (cur_node != NULL){
        if (cur_node->block_size_alloc >= size){
            memory_block_t *next_node = cur_node->next;
            size_t old_size = cur_node->block_size_alloc;
            memory_block_t *ret = split(cur_node, size); // *** double check these size values
            memory_block_t *new_node = split_new(cur_node, size, old_size);
            new_node->next = next_node;
            // ****work on case for perfect fit cases
            if(prev_node != NULL){
                prev_node->next = new_node;
            }
            if(prev_node == NULL){
                // first one got change
                free_head = new_node;
            }
            ret->next = new_node;
            return ret;
        }
        prev_node = cur_node;
        cur_node = cur_node->next;
    }
    // never found one
    return NULL;
}

/*
 * extend - extends the heap if more memory is required.
 */
memory_block_t *extend(size_t size) {
    return NULL;
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
    allocate(ret);
    return ret;
}

/*
 * split - return the left over portion back
 */
memory_block_t *split_new(memory_block_t *block, size_t size, size_t old_size) {
    memory_block_t *ret = (memory_block_t *) ((char *) block + size);
    ret->block_size_alloc = old_size - size;
    return ret;
}

/*
 * coalesce - coalesces a free memory block with neighbors.
 */
memory_block_t *coalesce(memory_block_t *block) {


    return NULL;
}

/*
 * print list for debugging
 */
void print_heap() {
    memory_block_t *cur_node = free_head;
    while(cur_node != NULL){
        printf("%s at %p, size %d\n",
                (is_allocated(cur_node))?"alllocated":"free",
                cur_node,
                (int) (cur_node->block_size_alloc));
        cur_node = cur_node->next;       
    }
}



/*
 * uinit - Used initialize metadata required to manage the heap
 * along with allocating initial memory.
 */
int uinit() {
    free_head = csbrk(PAGESIZE * 16); //*** intialize free_head to the max increment
    free_head->block_size_alloc = PAGESIZE * 16 - sizeof(memory_block_t); // set size to  PAGESIZE
    free_head->next = NULL; // next is pointing to NULL
    return 0;
}

/*
 * umalloc -  allocates size bytes and returns a pointer to the allocated memory.
 */
void *umalloc(size_t size) {
    memory_block_t *fitted_block = find(size + sizeof(memory_block_t));
    if (fitted_block == NULL){ // didn't find a block with enough space
        // call csbrk to make more
        memory_block_t *more_free = csbrk(PAGESIZE * 16);
        more_free->block_size_alloc = PAGESIZE * 16 - sizeof(memory_block_t); // - mem block 
        more_free->next = NULL;

        // now we split the result to two parts
        fitted_block = split(more_free, size);
        memory_block_t *new_free = split_new(more_free, size, 
                                        PAGESIZE * 16 - sizeof(memory_block_t));

        // now we link old free with the remainder
        memory_block_t *free_head_cur = free_head;
        while(free_head_cur->next != NULL){
            free_head_cur = free_head_cur->next;
        }
        free_head_cur->next = new_free;
    }
    // printf("***%s at %p, size %d, end address %p\n",
    //             (is_allocated(fitted_block))?"alllocated":"free",
    //             fitted_block,
    //             (int) get_size(fitted_block), fitted_block->next);  
    return fitted_block;
}

/*
 * ufree -  frees the memory space pointed to by ptr, which must have been called
 * by a previous call to malloc.
 */
void ufree(void *ptr) {
    //memory_block_t *cur_node = (memory_block_t *) ptr;

    //you have a cur_node
    // do insertion sorted in free_heap


}