# Host build and automated test foundation — milestone 3.8

## Objective

Provide a reproducible host-side build for the current DreamPSX core so compiler
errors and regression failures can be detected before beginning Dreamcast
hardware integration.

## Added files

- `CMakeLists.txt`: builds the current bus, IRQ, IRQ MMIO, memory, COP0,
  CPU and machine modules as the `dreampsx_core` static library.
- Registers twelve existing test programs with CTest.
- `.github/workflows/host-ci.yml`: configures CMake, compiles the project and
  runs CTest on pushes to `main`, pull requests targeting `main`, and manual
  workflow dispatch.
- README now documents the host build and its boundary relative to the
  future Dreamcast/KallistiOS target.

## Reproduce locally

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

## Validation status

The build configuration and CI workflow are committed. They have not been
executed in this editing environment. The first GitHub Actions run is the
next validation gate; compiler or test failures must be fixed from its logs.
No successful CI run, Dreamcast binary, CDI image or playable game is claimed.

## Next step

Inspect the first CI run, correct all compiler/test failures, and only then
treat the host core as validated. After that, expand the build toward the
Dreamcast/KallistiOS toolchain.
