# Dependencies

## Philosophy

heidi-kernel follows a **minimal dependencies** policy:
- Prefer standard library + small header-only libraries
- No heavy frameworks
- All dependencies must be justified

## Current Dependencies

### Required (Build)

| Tool | Version | Purpose |
|------|---------|---------|
| CMake | 3.22+ | Build system |
| Ninja | Any | Generator |
| Clang | 15+ | Compiler |

### Optional (Development)

| Tool | Purpose |
|------|---------|
| clang-format | Code formatting |
| clang-tidy | Static analysis |
| clang | Compiler + sanitizers |

### Testing

Currently: none (custom test runner or shell scripts)

## Adding Dependencies

If you need to add a dependency:

1. **Justify**: Why is it needed? What does it solve?
2. **Minimal**: Prefer header-only or small libraries
3. **Pin**: Use specific versions/tags
4. **Document**: Add to this file

## FetchContent Example

If using CMake FetchContent:

```cmake
include(FetchContent)
FetchContent_Declare(
  doctest
  GIT_REPOSITORY https://github.com/doctest/doctest.git
  GIT_TAG v2.4.11
)
FetchContent_MakeAvailable(doctest)
```

## Platform Notes

- **Linux/WSL2**: All tools available via apt
- **macOS**: Install via Homebrew
- **Windows**: Use MSVC or clang-cl via Visual Studio
