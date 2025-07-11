#ifndef ALLOCATOR_INTERFACE_H
#define ALLOCATOR_INTERFACE_H

#include <stdlib.h>
#include "runtime.h"

typedef struct {
  const char* name;
  void* (* alloc)(size_t bytes);
  void (* dealloc)(void* ptr);
  void (* gc)(void);
  void (* scope_end)(void);
  AllocatorStats* (* get_stats)(void);
  void (* init)(void);
  void (* shutdown)(void);
  void (* inc_ref_count)(void* ptr, void* other);
  void (* dec_ref_count)(void* ptr, size_t offset);
} RuntimeAllocator;

const RuntimeAllocator* get_allocator_implementation(void);

#endif