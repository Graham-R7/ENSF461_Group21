#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>

#include "myalloc.h"

void* _arena_start_ = NULL;
size_t _arena_size = 0;

node_t* free_list_head = NULL;

// allocate memory
int myinit(size_t size) {
    size_t pagesize = sysconf(_SC_PAGESIZE);

    printf("Initializing arena\n");
    printf("...requested size %zu bytes\n", size);
    printf("...pagesize is %zu bytes\n", pagesize);

    // Check for invalid size argument
    if (size <= 0 || size > MAX_ARENA_SIZE) {
        printf("Error: Invalid size argument\n");
        return ERR_BAD_ARGUMENTS;
    }

    size_t adjusted_size = (size < pagesize) ? pagesize : ((size + pagesize - 1) / pagesize) * pagesize;
    printf("...adjusting size with page boundaries\n");
    printf("...adjusted size is %zu bytes\n", adjusted_size);

    printf("...mapping arena with mmap()\n");
    _arena_start_ = mmap(NULL, adjusted_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    if (_arena_start_ == MAP_FAILED) {
        perror("mmap failed");
        return ERR_SYSCALL_FAILED;
    }

    void* arena_end = (char*)_arena_start_ + adjusted_size;
    printf("...arena starts at %p\n", _arena_start_);
    printf("...arena ends at %p\n", arena_end);

    _arena_size = adjusted_size;

    free_list_head = (node_t*)_arena_start_;
    free_list_head->size = adjusted_size - sizeof(node_t);
    free_list_head->is_free = 1;
    free_list_head->fwd = NULL;
    free_list_head->bwd = NULL;

    return _arena_size;
}

// deallocate memory
int mydestroy() {
    if (_arena_start_ == NULL) {
        return ERR_UNINITIALIZED;
    }

    if (munmap(_arena_start_, _arena_size) == -1) {
        perror("munmap failed");
        return ERR_SYSCALL_FAILED;
    }

    _arena_start_ = NULL;
    _arena_size = 0;
    free_list_head = NULL;

    return 0;
}


void* myalloc(size_t size){
    /*
        myalloc creates a chunk of memory
        chunk has a header which is a node_t
        "body" is allocated for the application

        size of the chunk MUST include the size of node_t i.e header
        Do not return header to program

        HEADERS:
        doubly linked list
        headers are allocated in chunk
        header points to next and previous chunk
        keep headers of allocated and free chunks
    */
   printf("Allocating memory:\n");
   size_t total_size = size + sizeof(node_t); // size of chunk
   node_t* current = free_list_head;

    printf("...looking for free chunk of >= %zu bytes\n", total_size);
    while(current != NULL){
        if(current->is_free && current->size >= total_size){
            node_t* new_block = (node_t*)((char*)current + total_size);
            new_block->size = current->size - total_size;
            new_block->is_free = 1;
            new_block->fwd = current->fwd;
            new_block->bwd = current;
            current->fwd = new_block;
            current->size = size;

            printf("...found free chunk of %zu bytes with header at %p\n",total_size, (void*)new_block);


            current->is_free = 0;
            return (void*)((char*)current + sizeof(node_t));
        }
        current = current->fwd;
    }
    return NULL;
}


void myfree(void* ptr) {
    if (ptr == NULL) return;

    // Get the node_t header
    node_t *block = (node_t*)((char*)ptr - sizeof(node_t));
    block->is_free = 1; // Mark as free

    // Coalesce adjacent free blocks if possible
    if (block->fwd != NULL && block->fwd->is_free) {
        // Coalesce with the next block
        block->size += sizeof(node_t) + block->fwd->size;
        block->fwd = block->fwd->fwd; // Update forward link
        if (block->fwd != NULL) {
            block->fwd->bwd = block; // Update backward link of next block
        }
    }
    if (block->bwd != NULL && block->bwd->is_free) {
        // Coalesce with the previous block
        block->bwd->size += sizeof(node_t) + block->size;
        block->bwd->fwd = block->fwd; // Update forward link
        if (block->fwd != NULL) {
            block->fwd->bwd = block->bwd; // Update backward link of next block
        }
    }
}

