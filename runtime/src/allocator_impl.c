#include "allocator_interface.h"

const RuntimeAllocator* get_allocator_implementation(void)
{
#if defined(USE_MARK_SWEEP)
#include "mark_sweep_allocator.h"
    return get_mark_sweep_allocator();
#elif defined(USE_REF_COUNT)
#include "reference_count_allocator.h"
    return get_reference_count_allocator();
#else
#include "simple_allocator.h"
    return get_simple_allocator();
#endif
}