#include "mark_sweep_allocator.h"

static const RuntimeAllocator mark_sweep_allocator = {
        .name = "Mark-Sweep GC",
};

const RuntimeAllocator* get_mark_sweep_allocator(void)
{
    return &mark_sweep_allocator;
}