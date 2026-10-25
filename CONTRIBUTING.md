# Contributing

## Scope

This repository contains generic embedded C infrastructure only. Do not submit:

- product protocols or command catalogs
- board addresses, register maps, or calibration data
- firmware images, bitstreams, XSA files, or vendor-generated projects
- customer names, credentials, keys, or private test data

## Required changes

- Keep the public API small and documented in English.
- Add or update host tests for every behavior change.
- Preserve C99 compatibility unless a proposal explicitly changes it.
- Comment non-obvious control flow, ownership, retry, and boundary logic in English.
- Avoid dynamic allocation and platform-specific dependencies in core modules.
- Run the build and tests before opening a pull request.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

By contributing, you confirm that you have the right to submit the code under this repository's MIT license.
