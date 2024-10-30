#ifndef ALLOCATOR_INTERFACE_H
#define ALLOCATOR_INTERFACE_H

#include <stdlib.h>
#include <stdio.h>
#include "runtime.h"

// Interface that all allocators must implement
typedef struct {
    const char* name;
    void* (*alloc)(size_t bytes);
    void (*dealloc)(void* ptr);
    void (*gc)(void);
    void (*scope_end)(void);
    AllocatorStats* (*get_stats)(void);
    void (*init)(void);
    void (*shutdown)(void);
} RuntimeAllocator;

const RuntimeAllocator* get_allocator_implementation(void);

#endif // ALLOCATOR_INTERFACE_H