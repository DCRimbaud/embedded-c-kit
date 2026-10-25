# Embedded C Kit

Portable, allocation-free C building blocks for embedded systems.

This repository is an extraction candidate from production firmware. It contains
only generic infrastructure and intentionally excludes product protocols, board
configuration, register maps, calibration data, and application logic.

## Included

- `ringbuf_t`: fixed-capacity, caller-owned byte ring buffer
- explicit count, so empty and full states are unambiguous
- drop-newest policy with a cumulative drop counter
- peek and clamped bulk-drop operations for frame parsers
- no heap allocation and no platform dependency
- `ec_mmio_read32` / `ec_mmio_write32` volatile 32-bit MMIO accessors
- `ec_tx_arbiter_*` frame/text ownership over a caller-provided byte writer
- optional Xilinx UARTLite adapter compatible with the same byte-writer callback
- fixed-capacity buffer pool with generation tokens and stale-release rejection
- allocation-free logger with level filtering, nested quiet windows, and sink callbacks
- transfer descriptors with ownership/release callbacks and a fixed descriptor queue
- alignment-safe big/little-endian 16/32/64-bit byte helpers

## Composition model

Modules are responsibility and test boundaries, not separately deployable services. A firmware image can
link a single module or the umbrella `embedded_c_kit` target. Modules may share the package's stable public
contracts and may be composed into a platform bundle; they must not duplicate common types or form dependency
cycles. See [ARCHITECTURE.md](https://github.com/DCRimbaud/embedded-c-kit/blob/main/ARCHITECTURE.md).

## Start here

- Read [ADOPTION.md](https://github.com/DCRimbaud/embedded-c-kit/blob/main/ADOPTION.md) to choose package,
  module, or example reuse.
- Read [the device-link example](https://github.com/DCRimbaud/embedded-c-kit/blob/main/examples/device_link/README.md)
  for an end-to-end host-runnable composition.

## Build and test

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

## API

```c
#include "embedded_c/ringbuf.h"

uint8_t storage[256];
ringbuf_t ring;

ringbuf_init(&ring, storage, sizeof(storage));
ringbuf_push(&ring, 0x55u);
```

After installation, consume the package with:

```cmake
find_package(embedded_c_kit CONFIG REQUIRED)
target_link_libraries(app PRIVATE embedded_c_kit::embedded_c_kit)
# Or link an individual module, for example embedded_c_kit::ringbuf
```

To build the optional Xilinx UARTLite adapter, point CMake at the BSP include directory:

```sh
cmake -S . -B build -DEMBEDDED_C_KIT_BUILD_XILINX_ADAPTERS=ON \\
  -DEMBEDDED_C_KIT_XILINX_BSP_INCLUDE_DIR=<bsp-include>
```

The adapter is disabled by default and does not copy AMD driver sources.

The ring is not internally synchronized. A common deployment is one producer and one consumer with external serialization for reset or reinitialization.

## Scope and exclusions

This repository must not contain:

- vendor or board-specific register maps
- product communication protocols
- customer configuration
- production firmware images
- calibration or tuning data
- non-public organizational test vectors

## License

MIT. See `LICENSE`.

## Code style

Public API and implementation comments are written in English.

## Author

Xue Weisen  
Email: xueweisen96@gmail.com  
GitHub: https://github.com/DCRimbaud
