#pragma once

#include "../duktape-2.7.0/src/duk_config.h"

#include <stdint.h>
#include <time.h>

#ifndef DUK_USE_INTERRUPT_COUNTER
#define DUK_USE_INTERRUPT_COUNTER
#endif

#ifdef DUK_USE_CALLSTACK_LIMIT
#undef DUK_USE_CALLSTACK_LIMIT
#endif
#define DUK_USE_CALLSTACK_LIMIT 1000

typedef struct krbn_duktape_timeout_state {
  uint64_t deadline_ns;
  int timed_out;
} krbn_duktape_timeout_state;

typedef struct krbn_duktape_heap_state {
  krbn_duktape_timeout_state timeout;
  uint64_t memory_limit_bytes;
  uint64_t memory_used_bytes;
  int memory_exceeded;
} krbn_duktape_heap_state;

static inline uint64_t krbn_duktape_get_monotonic_ns(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return ((uint64_t)ts.tv_sec * 1000000000ULL) + (uint64_t)ts.tv_nsec;
}

static inline int krbn_duktape_exec_timeout_check(krbn_duktape_timeout_state* state) {
  if (!state || state->deadline_ns == 0) {
    return 0;
  }
  if (krbn_duktape_get_monotonic_ns() > state->deadline_ns) {
    state->timed_out = 1;
    return 1;
  }
  return 0;
}

#ifdef DUK_USE_EXEC_TIMEOUT_CHECK
#undef DUK_USE_EXEC_TIMEOUT_CHECK
#endif
#define DUK_USE_EXEC_TIMEOUT_CHECK(udata) \
  krbn_duktape_exec_timeout_check(&((krbn_duktape_heap_state*)(udata))->timeout)
