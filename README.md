# heidi-kernel

A native C++23 runtime daemon for WSL2-first environments. Bounded queues, backpressure, deterministic scheduling, and clean shutdown.

## Quickstart

### Prerequisites

- C++23 compatible compiler (GCC 13+ or Clang 17+)
- CMake 3.22+
- Ninja build system

### Build

```bash
# Clone and setup
git clone https://github.com/heidi-dang/heidi-kernel.git
cd heidi-kernel
git submodule update --init --recursive

# Build
cmake --preset debug
cmake --build --preset debug
```

### Run

```bash
# Run the kernel (press Ctrl+C to stop)
./build/debug/heidi-kernel

# With options
./build/debug/heidi-kernel --log-level debug
./build/debug/heidi-kernel --version
./build/debug/heidi-kernel --help
```

### Test

```bash
# Run tests
ctest --preset debug

# Or run tests directly
./build/debug/heidi-kernel-tests
```

### Development

```bash
# Run smoke test
./scripts/smoke.sh   # Linux/macOS
.\scripts\smoke.ps1  # Windows
```

## Project Structure

- `src/` - Core library (logging, event loop, config, result types)
- `app/` - Main binary
- `include/` - Public headers
- `tests/` - Unit tests
- `scripts/` - Build and test scripts
- `.local/` - Private governance (submodule)

## License

Licensed under the MIT License. See LICENSE.
