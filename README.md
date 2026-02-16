# heidi-kernel

A native C++23 runtime daemon for WSL2-first environments. Bounded queues, backpressure, deterministic scheduling, and clean shutdown.

## Quick Start

```bash
# Clone with submodules
git clone --recurse-submodules https://github.com/heidi-dang/heidi-kernel
cd heidi-kernel

# Build
cmake --preset debug
cmake --build --preset debug

# Run
./build/debug/heidi-kernel --help
```

## Documentation

- [Developer Quickstart](docs/DEV_QUICKSTART.md) - Setup, build, test, format
- [CI Overview](docs/CI_OVERVIEW.md) - What runs on PR, nightly, release
- [Dependencies](docs/DEPENDENCIES.md) - Build tools and libraries
- [Release Process](docs/RELEASE.md) - How to create a release
- [Branch Protection](docs/branch_protection.md) - Recommended settings

## License

Licensed under the MIT License. See LICENSE.
