# Device Link Example

This example is a complete, host-runnable reference for composing the C kit in a firmware-style data path.
It is product-neutral and requires no development board.

## What it demonstrates

- `ringbuf` receives bytes and preserves an incomplete frame for the next poll.
- `bytes` reads the big-endian sequence field without alignment assumptions.
- `buf_pool` leases fixed response storage with generation-checked release.
- `xfer` and `xfer_queue` transfer descriptor ownership without copying payloads.
- `tx_arbiter` serializes frame output and resumes after writer backpressure.
- `log` records malformed, unsupported, and resource-exhaustion paths.
- A single composition root wires the modules without dynamic allocation.

## Frame format

| Field | Size | Meaning |
|---|---:|---|
| SOF | 1 | Fixed `0x5A` |
| Command | 1 | Request or response identifier |
| Sequence | 2 | Big-endian correlation value |
| Payload length | 1 | `0..32` |
| Payload | N | Product-neutral demonstration bytes |
| Checksum | 1 | XOR from command through the final payload byte |

Supported requests are `0x01` ping, `0x02` echo, and any unsupported command. Responses use the request
command with bit 7 set, except unsupported commands, which return `0xFF`.

## Reuse guidance

Copy the structure, not the command values. In a real project:

1. keep the ring, pool, queue, and descriptor boundaries;
2. replace the demonstration command IDs and payload schema in the application protocol module;
3. bind the arbiter writer to `xilinx_uartlite` or the project's transport adapter;
4. call `device_link_poll()` from the consumer context and `device_link_flush()` from the single TX owner;
5. preserve the checksum or replace it with a production CRC while retaining malformed-frame recovery.

## Build and run

From the repository root:

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

The `device_link_example` executable prints diagnostic records followed by `device link example: PASS`.
Set `EMBEDDED_C_KIT_BUILD_EXAMPLES=OFF` when embedding the kit as a CMake subdirectory and examples are not
wanted.
