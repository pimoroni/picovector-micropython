#pragma once

// MicroPython configuration for the PicoVector core library.
//
// The core headers select this file automatically (via __has_include) when this
// component's directory is on the include path — see picovector.hpp. It wires
// the rasteriser and std::vector containers onto MicroPython's GC heap, then
// pulls in config_default.hpp for the shared, overridable knobs (PV_PROFILE,
// PV_DUAL_CORE, PV_WORKING_BUFFER_SIZE) — left at their defaults here and set,
// where needed, by picovector-micropython.cmake.

#include "runtime/mp_allocator.hpp"

#ifdef __cplusplus
extern "C" {
#endif

#include "py/mphal.h"

#if MICROPY_MALLOC_USES_ALLOCATED_SIZE
  void *m_malloc(size_t num_bytes);
  void *m_malloc_no_scan(size_t num_bytes);
  void *m_realloc(void *ptr, size_t old_num_bytes, size_t new_num_bytes);
  void m_free(void *ptr, size_t num_bytes);
#else
  void *m_malloc(size_t num_bytes);
  void *m_malloc_no_scan(size_t num_bytes);
  void *m_realloc(void *ptr, size_t new_num_bytes);
  void m_free(void *ptr);
#endif
#ifdef __cplusplus
}
#endif

#define PV_TICKS mp_hal_ticks_ms()
#define PV_STD_ALLOCATOR MPAllocator
#define PV_MALLOC m_malloc
#define PV_MALLOC_NO_SCAN m_malloc_no_scan
#define PV_FREE m_free
#define PV_REALLOC m_realloc

// The allocator above is MicroPython's tracing GC: blocks are reclaimed
// automatically once unreferenced. PicoVector must therefore NOT explicitly free
// or run finalisers that free — doing so double-frees the same block during
// gc_sweep (the finaliser and the sweep both release it), corrupting the heap.
// Frees/finalisers are guarded on this flag so GC builds skip them while non-GC
// builds (bare Pico / host C++, which own their allocations) keep them.
#define PV_GC_MANAGED 1