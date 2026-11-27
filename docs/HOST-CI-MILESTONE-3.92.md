# Host continuous integration foundation — milestone 3.92

## Delivered

Added a GitHub Actions workflow at `.github/workflows/ci.yml` that runs on pushes, pull requests, and manual dispatch.

The workflow:
1. Checks out the repository.
2. Configures the existing CMake project in Debug mode.
3. Builds every target, including the registered host test executables.
4. Runs the CTest suite with failure output enabled.

## Why this matters

The project maintainer can now obtain compiler and test feedback directly from GitHub without needing a local computer. This is especially useful while development is being coordinated from a phone.

## Validation status

The workflow definition is committed, but its first run must complete before any claim can be made about compilation or test success. Host CI does not validate Dreamcast/KallistiOS compilation, CDI generation, timing accuracy, or game compatibility.
