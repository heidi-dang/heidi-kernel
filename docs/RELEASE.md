# Release Process

## Prerequisites

- Access to heidi-kernel repository
- Permission to create tags

## Steps

### 1. Ensure main is ready

```bash
git checkout main
git pull origin main
```

### 2. Run CI locally

```bash
# Build and test
cmake --preset debug && cmake --build --preset debug
ctest --preset debug
cmake --preset release && cmake --build --preset release

# Format and lint
./scripts/format.sh
./scripts/lint.sh
```

### 3. Create a tag

Tags must follow semver: `vMAJOR.MINOR.PATCH`

```bash
git tag -a v1.0.0 -m "Release v1.0.0"
git push origin v1.0.0
```

### 4. Release workflow runs

Once the tag is pushed:
1. GitHub Actions triggers `release` workflow
2. Builds on ubuntu-latest, macos-latest, windows-latest
3. Creates release artifacts
4. Uploads to GitHub Release

### 5. Verify

Check the Release page: https://github.com/heidi-dang/heidi-kernel/releases

Artifacts should include:
- `heidi-kernel-ubuntu-x64.tar.gz`
- `heidi-kernel-macos-x64.tar.gz`
- `heidi-kernel-windows-x64.zip`

## Manual Release (if needed)

Use workflow_dispatch:
1. Go to Actions > release
2. Click "Run workflow"
3. Enter tag name

## Post-release

- Update version in relevant files
- Create next milestone
- Announce (if applicable)
