#include "jblang/runtime/runtime.h"
#include "jblang/runtime/allocator_interface.h"
#include <stdio.h>
#include <stddef.h>

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

void runtime_inc_ref_count(void * ptr, void *other) {
    if (current_allocator->inc_ref_count) {
        current_allocator->inc_ref_count(ptr, other);
    }
}
void runtime_dec_ref_count(void * ptr, size_t offset) {
    if (current_allocator->dec_ref_count) {
        current_allocator->dec_ref_count(ptr, offset);
    }
}