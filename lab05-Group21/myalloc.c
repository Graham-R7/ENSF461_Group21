#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>

#include "myalloc.h"

void* _arena_start_ = NULL;
size_t _arena_size = 0;

node_t* free_list_head = NULL;
int statusno = 0;

// Initialize the arena with mmap and set up the free list
int myinit(size_t size) {
    size_t pagesize = sysconf(_SC_PAGESIZE);

    printf("Initializing arena\n");
    printf("...requested size %zu bytes\n", size);
    printf("...pagesize is %zu bytes\n", pagesize);

    if (size <= 0 || size > MAX_ARENA_SIZE) {
        printf("Error: Invalid size argument\n");
        statusno = ERR_BAD_ARGUMENTS;
        return ERR_BAD_ARGUMENTS;
    }

    size_t adjusted_size = (size < pagesize) ? pagesize : ((size + pagesize - 1) / pagesize) * pagesize;
    printf("...adjusting size with page boundaries\n");
    printf("...adjusted size is %zu bytes\n", adjusted_size);

    _arena_start_ = mmap(NULL, adjusted_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    if (_arena_start_ == MAP_FAILED) {
        perror("mmap failed");
        statusno = ERR_SYSCALL_FAILED;
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
    statusno = 0;

    return _arena_size;
}

// Destroy the arena and release the memory
int mydestroy() {
    if (_arena_start_ == NULL) {
        statusno = ERR_UNINITIALIZED;
        return ERR_UNINITIALIZED;
    }

    if (munmap(_arena_start_, _arena_size) == -1) {
        perror("munmap failed");
        statusno = ERR_SYSCALL_FAILED;
        return ERR_SYSCALL_FAILED;
    }

    _arena_start_ = NULL;
    _arena_size = 0;
    free_list_head = NULL;
    statusno = 0;

    return 0;
}

// Allocate a chunk of memory from the arena
void* myalloc(size_t size) {
    printf("Allocating memory:\n");

    if (_arena_start_ == NULL) {
        printf("...arena not initialized\n");
        statusno = ERR_UNINITIALIZED;
        return NULL;
    }

    size_t total_size = size + sizeof(node_t);
    node_t* current = free_list_head;

    printf("...looking for free chunk of >= %zu bytes\n", total_size);
    while (current != NULL) {
        if (current->is_free && current->size >= total_size) {
            printf("...found free chunk of %zu bytes with header at %p\n", current->size, (void*)current);

            if (current->size >= total_size + sizeof(node_t)) {
                printf("...splitting required\n");

                node_t* new_block = (node_t*)((char*)current + total_size);
                new_block->size = current->size - total_size;
                new_block->is_free = 1;
                new_block->fwd = current->fwd;
                new_block->bwd = current;

                if (current->fwd != NULL) {
                    current->fwd->bwd = new_block;
                }
                current->fwd = new_block;
                current->size = size;
            } else {
                printf("...splitting not required\n");
            }

            current->is_free = 0;
            statusno = 0;
            printf("...updating chunk header at %p\n", (void*)current);
            printf("...allocation starts at %p\n", (void*)((char*)current + sizeof(node_t)));
            return (void*)((char*)current + sizeof(node_t));
        }
        current = current->fwd;
    }

    printf("...no suitable chunk found\n");
    statusno = ERR_OUT_OF_MEMORY;
    return NULL;
}

// Free a previously allocated chunk
void myfree(void* ptr) {
    if (ptr == NULL) return;

    node_t *block = (node_t*)((char*)ptr - sizeof(node_t));
    block->is_free = 1;

    if (block->fwd != NULL && block->fwd->is_free) {
        block->size += sizeof(node_t) + block->fwd->size;
        block->fwd = block->fwd->fwd;
        if (block->fwd != NULL) {
            block->fwd->bwd = block;
        }
    }
    if (block->bwd != NULL && block->bwd->is_free) {
        block->bwd->size += sizeof(node_t) + block->size;
        block->bwd->fwd = block->fwd;
        if (block->fwd != NULL) {
            block->fwd->bwd = block->bwd;
        }
    }
}
