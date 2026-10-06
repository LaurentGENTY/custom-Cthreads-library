# Custom C threads library

A **user-level threads library** in C, API-compatible with a subset of `pthread`, with optional **preemptive scheduling** and benchmarks against the native `pthread` implementation.

> School project (operating systems), ENSEIRB-MATMECA. The full write-up with benchmark results is in [`rapport_final_G3_E5.pdf`](rapport_final_G3_E5.pdf) (French).

## Highlights

- **Context switching** with `ucontext` (`makecontext` / `swapcontext`) and a dedicated scheduler context.
- **Preemption** driven by a profiling timer (`setitimer`), with a configurable time slice (`TIMESLICE`, 15 ms by default; enable it by setting `USE_PREEMPTIVE` to `1` in `include/thread.h`).
- **Stack pool**: thread stacks are recycled instead of being reallocated on every `thread_create`.
- **Stack-overflow detection**: a guard page protected with `mprotect`, caught by a signal handler running on an alternate stack (`sigaltstack`).
- **Mutexes** with FIFO wait queues (`sys/queue.h`).
- **Valgrind support**: stacks are registered so Valgrind can track them.
- **Drop-in comparison**: the same tests compile against real `pthread` (`-DUSE_PTHREAD`) to compare performance, with gnuplot scripts to plot the results.

## API

```c
thread_t thread_self(void);
int  thread_create(thread_t *thread, void *(*func)(void *), void *arg);
int  thread_yield(void);
int  thread_join(thread_t thread, void **retval);
void thread_exit(void *retval);

int thread_mutex_init(thread_mutex_t *mutex);
int thread_mutex_destroy(thread_mutex_t *mutex);
int thread_mutex_lock(thread_mutex_t *mutex);
int thread_mutex_unlock(thread_mutex_t *mutex);
```

## Getting started

Requirements: Linux, `gcc`, `make`, Valgrind headers (`valgrind/valgrind.h`). Optional: `gnuplot`.

```bash
make            # build install/lib/libthread.so and the tests into install/bin
make check      # run the test suite against the library
make pthreads   # build the same tests against native pthread, for comparison
make valgrind   # run the tests under Valgrind
make graphs     # benchmark and plot custom threads vs pthread
```

## Tests

`test/` covers creation, yield, join, cascading switches, recursive workloads (Fibonacci, merge sort), mutex contention and a big parallel sum. Each benchmark has a matching `plotting_*.gpi` gnuplot script.

## Project structure

```
include/thread.h   # public API + pthread compatibility macros
src/thread.c       # create / yield / join / exit
src/utils.c        # scheduler, stack pool, preemption timer, guard pages
src/mutex.c        # mutexes
test/              # functional tests + gnuplot scripts
graphs/            # benchmark sources
```
