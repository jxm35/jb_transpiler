#include "mark_sweep_allocator.h"
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <stdio.h>

static void* stack_bottom = NULL;

static size_t GC_THRESHOLD = 1024*1024; // 1mb

typedef struct MSHeader {
  bool marked;
  size_t size;
  struct MSHeader* next;
  struct MSHeader* prev;
} MSHeader;

static MSHeader* allocation_list = NULL;
static AllocatorStats stats = {0};

static void add_allocation(MSHeader* header)
{
    header->next = allocation_list;
    header->prev = NULL;
    if (allocation_list) allocation_list->prev = header;
    allocation_list = header;
}

static void remove_allocation(MSHeader* header)
{
    if (header->prev) header->prev->next = header->next;
    else allocation_list = header->next;
    if (header->next) header->next->prev = header->prev;
}

static bool is_valid_pointer(void* ptr)
{
    if (!ptr) return false;

    MSHeader* current = allocation_list;
    while (current) {
        void* start = (void*) (current+1);
        void* end = (char*) start+(current->size-sizeof(MSHeader));
        if (ptr>=start && ptr<end) {
            return true;
        }
        current = current->next;
    }
    return false;
}

static MSHeader* find_header(void* ptr)
{
    if (!ptr) return NULL;

    MSHeader* current = allocation_list;
    while (current) {
        void* start = (void*) (current+1);
        void* end = (char*) start+(current->size-sizeof(MSHeader));
        if (ptr>=start && ptr<end) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

static void mark(void* ptr)
{
    MSHeader* header = find_header(ptr);
    if (!header || header->marked) return;

    header->marked = true;

    void** start = (void**) (header+1);
    void** end = (void**) ((char*) header+header->size);
    for (void** p = start; p<end; ++p) {
        if (is_valid_pointer(*p)) {
            mark(*p);
        }
    }
}

static void conservative_scan_stack(void)
{
    void** bottom = (void**) stack_bottom;

    void** stack_top = (void**) __builtin_frame_address(0);

#ifdef DEBUG
    printf("(debug) Scanning stack from %p to %p\n", stack_top, bottom);
#endif

    if (stack_top<bottom) {
        for (void** p = stack_top; p<bottom; ++p) {
            if (is_valid_pointer(*p)) {
                mark(*p);
            }
        }
    }
    else {
        for (void** p = bottom; p<stack_top; ++p) {
            if (is_valid_pointer(*p)) {
                mark(*p);
            }
        }
    }
}

static void mark_phase(void)
{
#ifdef DEBUG
    printf("(debug) Starting conservative mark phase\n");
#endif

    MSHeader* current = allocation_list;
    while (current) {
        current->marked = false;
        current = current->next;
    }

    conservative_scan_stack();
}

static void sweep_phase(void)
{
#ifdef DEBUG
    printf("(debug) Starting sweep phase\n");
#endif

    MSHeader* current = allocation_list;
    size_t freed_count = 0;
    size_t freed_bytes = 0;

    while (current) {
        MSHeader* next = current->next;
        if (!current->marked) {
            freed_count++;
            freed_bytes += current->size;
            stats.current_bytes -= current->size;
            stats.total_collections++;
            remove_allocation(current);
            free(current);
        }
        current = next;
    }

#ifdef DEBUG
    printf("(debug) Freed %zu objects (%zu bytes)\n", freed_count, freed_bytes);
#endif
}

static void collect_garbage(void)
{
#ifdef DEBUG
    printf("(debug) Starting garbage collection\n");
    size_t before = stats.current_bytes;
#endif

    mark_phase();
    sweep_phase();

#ifdef DEBUG
    printf("(debug) GC complete: %zu -> %zu bytes\n", before, stats.current_bytes);
#endif
}

static void* ms_alloc(size_t size)
{
    size_t total = sizeof(MSHeader)+size;
    MSHeader* header = malloc(total);
    if (!header) return NULL;

    header->marked = false;
    header->size = total;
    header->next = header->prev = NULL;

    add_allocation(header);
    stats.current_bytes += total;
    stats.total_allocations++;
    if (stats.current_bytes>stats.peak_bytes)
        stats.peak_bytes = stats.current_bytes;

#ifdef DEBUG
    printf("(debug) Allocated %zu bytes at %p\n", size, (void*)(header + 1));
#endif

    if (stats.current_bytes>GC_THRESHOLD) {
        collect_garbage();
    }

    return header+1;
}

static void ms_dealloc(void* ptr)
{
    if (!ptr) return;

    MSHeader* header = (MSHeader*) ptr-1;
    remove_allocation(header);
    stats.current_bytes -= header->size;
    free(header);
}

static void ms_gc(void)
{
    collect_garbage();
}

static void ms_scope_end(void)
{
}

static AllocatorStats* ms_get_stats(void)
{
    return &stats;
}

static void ms_init(void)
{
    stack_bottom = __builtin_frame_address(1);
    allocation_list = NULL;
    stats = (AllocatorStats) {0};

#ifdef DEBUG
    printf("(debug) Mark-sweep allocator initialized\n");
#endif
}

static void ms_shutdown(void)
{
    collect_garbage();
    while (allocation_list) {
        MSHeader* next = allocation_list->next;
#ifdef DEBUG
        printf("(debug) Shutdown freeing %p\n", next);
#endif
        free(allocation_list);
        allocation_list = next;
    }
}

static void ms_set_gc_threshold(size_t threshold)
{
    GC_THRESHOLD = threshold;
}

static const RuntimeAllocator mark_sweep_allocator = {
        .name = "Mark-Sweep GC",
        .alloc = ms_alloc,
        .dealloc = ms_dealloc,
        .gc = ms_gc,
        .scope_end = ms_scope_end,
        .get_stats = ms_get_stats,
        .init = ms_init,
        .shutdown = ms_shutdown,
        .inc_ref_count = NULL,
        .dec_ref_count = NULL,
        .set_gc_threshold = ms_set_gc_threshold
};

const RuntimeAllocator* get_mark_sweep_allocator(void)
{
    return &mark_sweep_allocator;
}
