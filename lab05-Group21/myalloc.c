#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>

#include "myalloc.h"

void* _arena_start_ = NULL;
size_t _arena_size = 0;

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
    return _arena_size;
}

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

    return 0;
}