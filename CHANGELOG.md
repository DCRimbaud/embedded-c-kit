# Changelog

## 0.3.0 - 2026-10-26

- added the `embedded_c_kit` umbrella CMake target for integrated use
- added a host-runnable device-link integration example with detailed function contracts
- added an adoption guide for choosing package, module, or example reuse
- linked the Vivado/Vitis platform template for BD, XSA, and Vitis project integration
- added transfer descriptors with borrowed/static/pooled/transferred ownership
- added a fixed descriptor queue that preserves ownership on full/error paths
- added alignment-safe big/little-endian integer read/write helpers
- adopted personal copyright, author headers, Gmail, and GitHub metadata
- renamed the repository to `embedded-c-kit`

## 0.2.0 - 2026-10-26

- fixed-capacity buffer pool with generation-token stale-release protection
- allocation-free logger with level filtering, nested quiet windows, and sink callbacks
- optional Xilinx UARTLite adapter with fake-BSP host coverage
- installable CMake package with downstream consumer verification
- SPDX MIT identifiers across source and test files

## 0.1.0 - 2026-10-26

- fixed-capacity caller-owned ring buffer
- volatile 32-bit MMIO accessors
- frame/text byte-transmit arbiter with backpressure retry semantics
- cross-platform Windows/Linux CI
- contribution, security, formatting, and English-comment conventions
