#include "reference_count_allocator.h"
#include <stdio.h>

// Implementation details...
static const RuntimeAllocator reference_count_allocator = {
        .name = "Reference-Count GC",
        // TODO: implement
};

const RuntimeAllocator* get_reference_count_allocator(void) {
    return &reference_count_allocator;
}