
#include "umalloc.h"
#include <assert.h>
#include <stdint.h>

//Place any variables needed here from umalloc.c as an extern.
extern memory_block_t *free_head;

/*
 * check_heap -  used to check that the heap is still in a consistent state.
 * Required to be completed for checkpoint 1.
 * Should return 0 if the heap is still consistent, otherwise return a non-zero
 * return code. Asserts are also a useful tool here.
 */
int check_heap() {
    assert(free_head != NULL);
    // 1. check if every block is marked as free
    memory_block_t *cur_node = free_head;
    while (cur_node != NULL){
        // if it's allocatied then retirn zero
        if (is_allocated(cur_node)){
            return 1;
        }
        cur_node = cur_node->next;
    }

    // 2. are payload in increasing order, before seeing if they are adjecent
    cur_node = free_head;
    memory_block_t *next_node = cur_node->next;
    while (next_node != NULL){
        if (cur_node > next_node){
            return 1;
        }
        cur_node = next_node;
        next_node = cur_node->next;
    }

    // 3. check if two block are next to each to eachother (should be coalescing)
    cur_node = free_head;
    next_node = cur_node->next;
    while (next_node != NULL){
        // add size of cur_node then add the size of the free block
        // check if it's going the be the same of the start of the next address
        char *ending_address = (char *) (cur_node + 1) + cur_node->block_size_alloc;
        if (ending_address == (char *) next_node){
            return 1;
        }
        cur_node = next_node;
        next_node = cur_node->next;
    }

    // 4. are all block aligned and payload is aligned correctly
    // should be a divisiable by 16 for header 
    cur_node = free_head;
    while (cur_node != NULL){
        // cast curnode to be safe
        assert((uintptr_t) cur_node % 16 == 0);
        if ((uintptr_t) cur_node % 16 != 0){
            return 1;
        }
        cur_node = cur_node->next;
    }
    // everything looks good
    return 0;
}

