#include "runtime.h"
#include "allocator_interface.h"
#include <stdio.h>
#include <stddef.h>

static const RuntimeAllocator* current_allocator = NULL;

void runtime_init(void)
{
    current_allocator = get_allocator_implementation();
    if (current_allocator) {
        current_allocator->init();
#ifdef DEBUG
        printf("(debug) Initialized runtime with %s\n", current_allocator->name);
#endif
    }
}

void runtime_shutdown(void)
{
    if (current_allocator) {
        current_allocator->shutdown();
#ifdef DEBUG
        runtime_print_stats();
#endif
        current_allocator = NULL;
    }
}

void* runtime_alloc(size_t bytes)
{
    return current_allocator ? current_allocator->alloc(bytes) : NULL;
}

void runtime_scope_end()
{
    if (current_allocator && current_allocator->scope_end) {
        current_allocator->scope_end();
    }
#ifdef DEBUG
    printf("(debug) scope ended\n");
#endif
}

void runtime_gc(void)
{
    if (current_allocator && current_allocator->gc) {
        current_allocator->gc();
    }
}

void runtime_set_gc_threshold(size_t threshold) {
    if (current_allocator && current_allocator->set_gc_threshold) {
        current_allocator->set_gc_threshold(threshold);
    }
}

void runtime_register_root(void* ptr) {
    if (current_allocator && current_allocator->register_root) {
        current_allocator->register_root(ptr);
    }
}

const char* runtime_get_allocator_name(void)
{
    return current_allocator->name;
}

AllocatorStats* runtime_get_stats(void)
{
    return current_allocator->get_stats();
}

void runtime_print_stats(void)
{
    AllocatorStats* stats = runtime_get_stats();
    printf("\n\nRuntime Stats (%s)\n", runtime_get_allocator_name());
    printf("Total allocs: %zu\nTotal collections: %zu\n", stats->total_allocations, stats->total_collections);
    printf("Current bytes: %zu\nPeak bytes: %zu\n", stats->current_bytes, stats->peak_bytes);
}

void runtime_inc_ref_count(void* ptr, void* other)
{
    if (current_allocator->inc_ref_count) {
        current_allocator->inc_ref_count(ptr, other);
    }
}

void runtime_dec_ref_count(void* ptr, size_t offset)
{
    if (current_allocator->dec_ref_count) {
        current_allocator->dec_ref_count(ptr, offset);
    }
}
