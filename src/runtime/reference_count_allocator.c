#include "jblang/runtime/reference_count_allocator.h"
#include <stdio.h>
#include <string.h>

static AllocatorStats stats = {0};

typedef void (*deallocatorFunc)(void *);
typedef struct deallocator {
    deallocatorFunc free;
    struct deallocator* next;
} deallocator;

typedef struct RefcountHeader {
    long long count; // bigger than needed, but ensures 8 byte alignment
    size_t size;
    void * to_free[3];
    int idx;
} RefcountHeader;

static void* rc_alloc(size_t bytes) {
    size_t size = bytes + sizeof(RefcountHeader);
    RefcountHeader * ptr = malloc(size);
    *ptr = (RefcountHeader) {
        .count = 1,
        .size = size,
        .to_free = {0, 0, 0},
        .idx = 0,
    };
    if (ptr) {
        stats.total_allocations++;
        stats.current_bytes += size;
        if (stats.current_bytes > stats.peak_bytes) {
            stats.peak_bytes = stats.current_bytes;
        }
    }
    return ptr+1;
}

static void inc_ref_count(void *ptr, void *other) {
    if (!ptr) return;
    RefcountHeader  *header = (RefcountHeader *) ((char *) ptr - sizeof(RefcountHeader));
    header->count++;
    if (other) {
        RefcountHeader  *otherHeader = (RefcountHeader *) ((char *) other - sizeof(RefcountHeader));
        if (otherHeader->idx < 3)
            otherHeader->to_free[otherHeader->idx++] = ptr;
    }
    printf("inc %lld\n", header->count);
}

static void dec_ref_count(void *ptr, size_t offset) {
    if (!ptr) return;
    RefcountHeader  *header = (RefcountHeader *) ((char *) ptr - offset - sizeof(RefcountHeader));
    header->count--;
    printf("dec %lld\n", header->count);
    if (header->count == 0) {
        for (int i = 0; i < header->idx; i++) {
            dec_ref_count(header->to_free[i], 0);
        }
        stats.total_collections++;
        stats.current_bytes -= header->size;
        free(header);
    }
}

static void rc_dealloc(void* ptr) {
    // No op
}

static void rc_gc(void) {
    // No-op
}

static void rc_scope_end(void) {
    // No-op
}

static AllocatorStats* rc_get_stats(void) {
    return &stats;
}

static void rc_init(void) {
    memset(&stats, 0, sizeof(stats));
}

static void rc_shutdown(void) {
}

static const RuntimeAllocator reference_count_allocator = {
        .name = "Reference-Count GC",
        .inc_ref_count = inc_ref_count,
        .dec_ref_count = dec_ref_count,
        .alloc = rc_alloc,
        .dealloc  = rc_dealloc,
        .gc = rc_gc,
        .scope_end = rc_scope_end,
        .get_stats = rc_get_stats,
        .init = rc_init,
        .shutdown = rc_shutdown
};

const RuntimeAllocator* get_reference_count_allocator(void) {
    return &reference_count_allocator;
}