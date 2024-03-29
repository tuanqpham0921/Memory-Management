#include <stdlib.h>
#include <stdbool.h>

#define ALIGNMENT 16 /* The alignment of all payloads returned by umalloc */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~(ALIGNMENT-1))

/*
 * memory_block_t - Represents a block of memory managed by the heap. The 
 * struct can be left as is, or modified for your design.
 * In the current design bit0 is the allocated bit
 * bits 1-3 are unused.
 * and the remaining 60 bit represent the size.
 */
typedef struct memory_block_struct { 
    size_t block_size_alloc; 
    struct memory_block_struct *next; // 8 bytes
    // for allocated next is end address
} memory_block_t;

// Helper Functions, this may be editted if you change the signature in umalloc.c
bool is_allocated(memory_block_t *block);
void allocate(memory_block_t *block);
void deallocate(memory_block_t *block);
size_t get_size(memory_block_t *block);

// my new functions
void print_heap();
memory_block_t *split_new(memory_block_t *new, memory_block_t *old, 
                                size_t old_size, memory_block_t *old_next);
void add_to_heap(memory_block_t *block);
void coalesce(memory_block_t *lead, memory_block_t *trailer, 
                                    size_t lead_end, size_t trailer_begin);

memory_block_t *find(size_t size);
memory_block_t *split(memory_block_t *block, size_t size);

// Portion that may not be edited
int uinit();
void *umalloc(size_t size);
void ufree(void *ptr);