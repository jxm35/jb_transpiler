#include "simple_allocator.h"
#include <stdio.h>
#include <string.h>

static AllocatorStats stats = {0};

static void* simple_alloc(size_t bytes) {
    void* ptr = malloc(bytes);
    if (ptr) {
        stats.total_allocations++;
        stats.current_bytes += bytes;
        if (stats.current_bytes > stats.peak_bytes) {
            stats.peak_bytes = stats.current_bytes;
        }
    }
    return ptr;
}

static void simple_dealloc(void* ptr) {
    free(ptr);
}

static void simple_gc(void) {
    // No-op
}

static void simple_scope_end(void) {
    // No-op
}

static AllocatorStats* simple_get_stats(void) {
    return &stats;
}

static void simple_init(void) {
    memset(&stats, 0, sizeof(stats));
}

static void simple_shutdown(void) {
}

static const RuntimeAllocator simple_allocator = {
        .name = "Simple Allocator",
        .alloc = simple_alloc,
        .dealloc = simple_dealloc,
        .gc = simple_gc,
        .scope_end = simple_scope_end,
        .get_stats = simple_get_stats,
        .init = simple_init,
        .shutdown = simple_shutdown
};

const RuntimeAllocator* get_simple_allocator(void) {
    return &simple_allocator;
}