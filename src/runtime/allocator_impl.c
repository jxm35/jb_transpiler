#include "jblang/runtime/allocator_interface.h"

// This file determines which allocator implementation to use based on compile flags
const RuntimeAllocator* get_allocator_implementation(void) {
#if defined(USE_MARK_SWEEP)
    #include "jblang/runtime/mark_sweep_allocator.h"
    return get_mark_sweep_allocator();
#elif defined(USE_REF_COUNT)
    #include "jblang/runtime/reference_count_allocator.h"
    return get_reference_count_allocator();
#else
#include "jblang/runtime/simple_allocator.h"
    return get_simple_allocator();
#endif
}