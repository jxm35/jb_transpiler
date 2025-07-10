#ifndef MARK_SWEEP_ALLOCATOR_H
#define MARK_SWEEP_ALLOCATOR_H

#include "jblang/runtime/allocator_interface.h"

const RuntimeAllocator* get_mark_sweep_allocator(void);

#endif // MARK_SWEEP_ALLOCATOR_H
