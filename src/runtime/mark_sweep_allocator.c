#include "jblang/runtime/mark_sweep_allocator.h"
#include <stdio.h>

// Implementation details...
static const RuntimeAllocator mark_sweep_allocator = {
        .name = "Mark-Sweep GC",
        // TODO: implement
};

const RuntimeAllocator* get_mark_sweep_allocator(void) {
    return &mark_sweep_allocator;
}