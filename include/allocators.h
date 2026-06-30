//my intentions with this header is to have a big stockpile of custom mem allocation tools to help
//make mem management easier and more ergonomic. Right now there's only an arena that isn't even all 
//that feature rich, so later I'll ahd more alloc types and more features to each alloc type.
//
//
//This is meant for the BorealOS project, but I (m4rn-progs) will be taking this to my own personal repos for personal use.
//
//anyone else, feel free to take a copy of this header file for your own projects. Ain't even my code to begin with lol


#ifndef ALLOCATORS_H
#define ALLOCATORS_H

#include <stddef.h>
#include <stdint.h>

// --- Configuration & Macros ---

#define DEFAULT_ALIGNMENT (2 * sizeof(void *))
#define is_power_of_two(x) (((x) != 0) && (((x) & ((x) - 1)) == 0))

// Syntactic sugar for typed allocations
#define make(T, n, a) ((T *)((a).alloc(sizeof(T) * (n), (a).context)))
#define release(s, p, a) ((a).free((s), (p), (a).context))

// Helper to quickly bind an Arena to the generic Allocator interface
#define arena_alloc_init(a) (Allocator){arena_alloc, arena_free, (a)}

// --- Types ---
//struct definition of an Arena.
typedef struct {
    void *base;
    size_t size;
    size_t offset;
    size_t committed;
} Arena;

//I've (the person who I borrowed this from has) seperated the idea of an allocator from an arena so that
//they could one day switch different types of allocators in. 

typedef struct {
    void *(*alloc)(size_t size, void *context);
    void (*free)(size_t size, void *ptr, void *context);
    void *context;
} Allocator;


// --- For Arenas ---
// create a brand new arena. Doesn't not allocate mem from OS (gotta do it urself)
static inline Arena arena_init(void *buffer, size_t size) {
    return (Arena){
        .base = buffer, 
        .size = size, 
        .offset = 0, 
        .committed = 0
    };
}
// ensure that all bits of memory take up space of a power of two; the cpu hates us if we don't
static inline uintptr_t align_forward(uintptr_t ptr, size_t alignment) {
    uintptr_t p, a, modulo;
    if (!is_power_of_two(alignment)) {
        return 0;
    }

    p = ptr;
    a = (uintptr_t)alignment;
    // doing bitwise math here to save a few cycles
    modulo = p & (a - 1);

    if (modulo) {
        p += a - modulo;
    }

    return p;
}


// the thing that does the thing; check for space, check for enough room, align, allocate
static inline void *arena_alloc_aligned(Arena *a, size_t size, size_t alignment) {
    uintptr_t curr_ptr = (uintptr_t)a->base + (uintptr_t)a->offset;
    uintptr_t offset = align_forward(curr_ptr, alignment);
    offset -= (uintptr_t)a->base;
    
    // Check for enough mem
    if (offset + size > a->size) {
        return 0; // Out of memory
    }

    a->committed += size;
    void *ptr = (uint8_t *)a->base + offset;
    a->offset = offset + size;

    return ptr;
}

//Wrapper for the function above

static inline void *arena_alloc(size_t size, void *context) {
    if (!size) {
        return 0;
    }
    return arena_alloc_aligned((Arena *)context, size, DEFAULT_ALIGNMENT);
}

// This function below does nothing by design.
/*  
 * Most libraries will ask for a function pointer for a free-mem function to cover for allocator differences cuz it'd
 * be impossible to know if its being used in a gen alloc or in an arena like this one.
 * So, we'll provide a fake free function to satify whatever library we end up using because in an arena we don't free
 * until the very end of the arena's lifespan.
 */
static inline void arena_free(size_t size, void *ptr, void *context) {
    (void)ptr; (void)size; (void)context;
}

//O(1) clears the entire arena

static inline void arena_free_all(void *context) {
    Arena *a = (Arena *)context;
    a->offset = 0;
    a->committed = 0;
}

// securely clear the entire arena. O(n)
#include <string.h>

static inline void arena_free_all_secure(void *context) {
    Arena *a = (Arena *)context;
    //wipe everything to zeros
    if (a->offset > 0) {
        memset(a->base, 0, a->offset);
        //make sure that whatever compiler I use doesn't remove the memset instructino
#if defined(__GNUC__) || defined(__clang__)
        __asm__ __volatile__("" : : "r"(a->base) : "memory");
#endif
    }
    
    a->offset = 0;
    a->committed = 0;
}

#endif // ALLOCATORS_H
