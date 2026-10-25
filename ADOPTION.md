# Adoption Guide

This guide is the first stop for a new embedded project. Choose the smallest integration that solves the
problem; do not copy the whole repository by default.

## Decision table

| Situation | Recommended action | Why |
|---|---|---|
| New MicroBlaze/Zynq project needs buffers, MMIO, logging, or packet ownership | link `embedded_c_kit::embedded_c_kit` | one stable target with selective module targets still available |
| Existing project needs only one mechanism | link the matching target, such as `embedded_c_kit::ringbuf` | avoids unrelated code and build cost |
| Need a UART command/frame path | copy `examples/device_link` and replace protocol values | preserves tested composition, ownership, and backpressure behavior |
| Need Xilinx UARTLite integration | enable `EMBEDDED_C_KIT_BUILD_XILINX_ADAPTERS` and bind the official BSP | keeps vendor types outside portable headers |
| Need host tests without hardware | compile the real source files with a fake BSP or scripted writer | tests production logic, not a copied implementation |
| Need a project-specific protocol | add it above this kit; do not put product IDs in the public modules | keeps the baseline reusable and safe to publish |

## CMake integration

### Installed package

```cmake
find_package(embedded_c_kit CONFIG REQUIRED)
target_link_libraries(firmware PRIVATE embedded_c_kit::embedded_c_kit)
```

### Source checkout

```cmake
add_subdirectory(path/to/embedded-c-kit)
target_link_libraries(firmware PRIVATE embedded_c_kit::embedded_c_kit)
```

Examples default to enabled only when the kit is the top-level CMake project. When it is embedded as a
subdirectory, they default to off; set `EMBEDDED_C_KIT_BUILD_EXAMPLES=ON` only when wanted.

## Module selection

| Need | Start with | Notes |
|---|---|---|
| Byte stream buffering | `ringbuf` | caller owns storage; no internal lock |
| Register access | `mmio` | volatile 32-bit access; keep address policy outside |
| Endian conversion | `bytes` | unaligned-safe read/write helpers |
| Frame/text TX exclusion | `tx_arbiter` | bind a transport writer callback |
| Fixed payload reuse | `buf_pool` | generation token rejects stale releases |
| Payload ownership | `xfer`, `xfer_queue` | descriptors carry release callback and context |
| Diagnostic events | `log` | bind a sink; do not print inside business logic |
| Xilinx UARTLite | `xilinx_uartlite` | optional adapter over the official BSP |

## Reuse checklist

1. Define the application's data owner, task/interrupt context, and maximum payload size.
2. Select the smallest target or copy the example only when composition is needed.
3. Bind transport writers, log sinks, and release callbacks in one composition root.
4. Document timeout, backpressure, stale-handle, and malformed-frame behavior in the project contract.
5. Test normal, full, empty, partial, timeout, and failure paths on the host.
6. Verify stack, static RAM, cache maintenance, interrupt latency, and electrical timing on the target.
7. Keep product protocols, board maps, calibration, and customer test vectors in the private project.

## What not to do

- Do not copy every module into every project.
- Do not duplicate shared types or error mappings to make modules look independent.
- Do not put `xuartlite.h` or FreeRTOS types into portable public headers.
- Do not add a function-pointer table without a real replacement or multi-instance boundary.
- Do not treat a successful host build as proof of target timing or cache correctness.

## Reference paths

- [Architecture](ARCHITECTURE.md)
- [Device link example](examples/device_link/README.md)
- [Repository](https://github.com/DCRimbaud/embedded-c-kit)
