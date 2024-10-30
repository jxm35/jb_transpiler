#include "runtime.h"
#include "allocator_interface.h"
#include <stdio.h>


static const RuntimeAllocator* current_allocator = NULL;

void runtime_init(void) {
    current_allocator = get_allocator_implementation();
    if (current_allocator) {
        current_allocator->init();
        printf("Initialized runtime with %s\n", current_allocator->name);
    }
}

void runtime_shutdown(void) {
    if (current_allocator) {
        current_allocator->shutdown();
        runtime_print_stats();
        current_allocator = NULL;
    }
}

void* runtime_alloc(size_t bytes) {
    return current_allocator ? current_allocator->alloc(bytes) : NULL;
}

void runtime_scope_end() {
    printf("scope ended\n");
//    runtime_gc();
}

const char* runtime_get_allocator_name(void) {
    return current_allocator->name;
}
AllocatorStats* runtime_get_stats(void) {
    return current_allocator->get_stats();
}
void runtime_print_stats(void) {
    AllocatorStats *stats = runtime_get_stats();
    printf("\n\nRuntime Stats\n");
    printf("Total allocs: %zu\nTotal collections: %zu\n", stats->total_allocations, stats->total_collections);
    printf("Current bytes: %zu\nPeak bytes: %zu\n", stats->current_bytes, stats->peak_bytes);
}