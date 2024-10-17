#ifndef __myalloc_h__
#define __myalloc_h__

// Error codes
#define ERR_OUT_OF_MEMORY  (-1)
#define ERR_BAD_ARGUMENTS  (-2)
#define ERR_SYSCALL_FAILED (-3)
#define ERR_CALL_FAILED    (-4)
#define ERR_UNINITIALIZED  (-5)

// Maximum arena size
#define MAX_ARENA_SIZE (0x7FFFFFFF)  // 2GB limit for arena

extern int statusno;  // Error status number

// Function prototypes
extern int myinit(size_t size);
extern int mydestroy();
extern void* myalloc(size_t size);
extern void myfree(void* ptr);

// Node structure to track memory blocks
typedef struct __node_t {
    size_t size;
    unsigned short is_free;
    struct __node_t* fwd;
    struct __node_t* bwd;
} node_t;

#endif  // __myalloc_h__
