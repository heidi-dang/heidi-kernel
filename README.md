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

## Getting Started

New developers should read the [Onboarding Guide](docs/ONBOARDING.md) for setup instructions.

## Documentation

- [Developer Quickstart](docs/DEV_QUICKSTART.md) - Setup, build, test, format
- [CI Overview](docs/CI_OVERVIEW.md) - What runs on PR, nightly, release
- [Dependencies](docs/DEPENDENCIES.md) - Build tools and libraries
- [Release Process](docs/RELEASE.md) - How to create a release
- [Branch Protection](docs/branch_protection.md) - Recommended settings
- [Onboarding Guide](docs/ONBOARDING.md) - First-time setup
- [Troubleshooting](docs/TROUBLESHOOTING.md) - Common issues and solutions
- [Architecture](docs/ARCHITECTURE.md) - System design
- [Process Policy](docs/PROCESS_POLICY.md) - Development workflow
- [WSL2 Setup](docs/WSL2.md) - Environment configuration

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md) for contribution guidelines and requirements.

## License

Licensed under the MIT License. See LICENSE.
