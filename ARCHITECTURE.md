# Embedded C Kit Architecture

## Purpose

This repository provides portable infrastructure for embedded C applications. It intentionally excludes
product protocols, board maps, calibration data, application policy, and vendor driver implementations.

The kit is consumed as source or as a CMake package. Its modules are designed for host testing and selective
linking, but they are not independent processes, plugins, or mandatory separate deliverables. A production
firmware normally links several modules into one image.

## Composition model

An **independent module** in this repository means:

- it has one primary reason to change;
- callers depend only on its public header contract;
- its core behavior can be tested without target hardware;
- a real alternative implementation can be substituted without rewriting every caller.

It does not mean zero dependencies. Shared ownership descriptors, byte helpers, and callback contracts are
expected to be reused. The recommended integration forms are:

1. link `embedded_c_kit::embedded_c_kit` for the complete portable kit;
2. link selected targets such as `embedded_c_kit::ringbuf` and `embedded_c_kit::xfer` for a smaller image;
3. place a platform-specific adapter above the portable modules without copying their logic;
4. let the application composition root create objects, bind callbacks, and define task or interrupt context.

Do not copy a common type, error mapping, logger, or byte-order helper just to make a module appear
self-contained. A duplicated fact is a coupling failure even when the directories look independent.

## Module map

| Module | Responsibility | Direct relationship |
|---|---|---|
| `ringbuf` | fixed-capacity byte ring with explicit counts | C standard library only |
| `mmio` | volatile 32-bit memory access | C standard library only |
| `bytes` | alignment-safe endian helpers | C standard library only |
| `buf_pool` | fixed payload slots and stale-release protection | C standard library only |
| `log` | allocation-free level filtering and sink callback | caller-provided sink |
| `tx_arbiter` | frame/text ownership over a byte writer | caller-provided writer callback |
| `xfer` | payload ownership and release contract | target-provided release callback |
| `xfer_queue` | fixed-capacity descriptor queue | `xfer` |
| `xilinx_uartlite` | optional Xilinx UARTLite adapter | `tx_arbiter` and the selected Xilinx BSP |

The modular map is a dependency guide, not a rule that every module must remain isolated. More capable
modules may compose these primitives when the resulting API has a clear responsibility and its dependencies
remain one-way.

## Dependency rules

- Portable modules may depend only on the C standard library and public contracts from this package.
- Vendor headers belong only in optional adapters, never in portable public headers.
- A lower-level primitive must not depend on a higher-level application policy.
- Include and link graphs must remain acyclic.
- Cross-module callbacks carry an explicit `context` pointer.
- A shared fact has one owner. If several modules need it, move it to the smallest stable public contract.
- Prefer direct calls inside a module. Introduce a function-pointer table only for a real replacement,
  host-fake, or multi-instance boundary.
- The application composition root owns platform initialization, callback binding, task creation, and teardown.

## Data ownership

Every API that accepts a pointer must make lifetime and mutation rules clear:

- borrowed read-only data uses `const T *` and remains valid only for the documented call or descriptor lifetime;
- output buffers use `T *` with an explicit capacity;
- static data is documented as read-only and non-owning;
- owned payloads carry a release callback and context;
- queue operations copy the descriptor, not the payload;
- `void *` is limited to callback context, type-erased adapter boundaries, and controlled containers.

C code does not infer ownership from a pointer type. The public contract must state who allocates, who may
mutate, who releases, and what happens after a failed operation.

## Concurrency and RTOS boundary

The portable modules do not create tasks and generally do not lock internally. Their headers state whether
the caller must serialize access. ISR/task handoff, timeouts, mutexes, and scheduler startup belong to the
platform or application layer.

A future or project-specific OS adapter may provide queues and time services, but vendor RTOS types must not
appear in portable public headers. An adapter can wrap FreeRTOS while preserving millisecond timeouts and
explicit status codes at the kit boundary.

## Extending the kit

A new module is accepted when all of the following hold:

1. it has a product-neutral responsibility;
2. its public header is self-contained and documents ownership and failure behavior;
3. host tests cover normal, boundary, and failure paths;
4. it does not introduce a dependency cycle or vendor type into portable headers;
5. it can be linked selectively and also works through the umbrella target;
6. its integration story explains which existing module or platform adapter owns composition.

Related modules should be composed rather than forced into artificial independence. If two modules always
change for the same reason and call each other in both directions, merge them or extract the shared stable
contract instead of adding an event bus or global registry.

## Validation

The normal validation sequence is:

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

Enable `EMBEDDED_C_KIT_BUILD_XILINX_ADAPTERS` with a real or fake BSP include directory to validate optional
vendor adapters. Host tests prove portable behavior; target builds and board tests remain responsible for
memory budget, interrupt latency, cache maintenance, and electrical timing.
