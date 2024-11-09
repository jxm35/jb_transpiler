#ifndef RUNTIME_H
#define RUNTIME_H

#include <stdlib.h>
#include <stdbool.h>

// Core runtime interface
void* runtime_alloc(size_t bytes);
//void runtime_dealloc(void* ptr);
//void runtime_gc(void);
void runtime_scope_end(void);
void runtime_init(void);
void runtime_shutdown(void);

void runtime_inc_ref_count(void * ptr, void *other);
void runtime_dec_ref_count(void * ptr, size_t offset);

typedef struct {
    size_t total_allocations;
    size_t current_bytes;
    size_t peak_bytes;
    size_t total_collections;
} AllocatorStats;

const char* runtime_get_allocator_name(void);
AllocatorStats* runtime_get_stats(void);
void runtime_print_stats(void);

#endif // RUNTIME_H