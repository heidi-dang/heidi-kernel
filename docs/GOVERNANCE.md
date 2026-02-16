# Governance Policy

## Automatic Enforcement (CI)

The following checks run automatically on every PR:

### 1. Emoji Check
- **Blocks**: PR titles/bodies with emoji characters (🧪, ⚡, 🔒, etc.)
- **Why**: Prevents cross-platform compatibility issues and keeps history clean
- **Applies to**: New commits only (not historical commits on main)

### 2. Build Artifact Check
- **Blocks**: Files matching `*.o`, `*.a`, `*.d`, `*.obj`, `*.pdb`, `*.exe`, `build/`
- **Why**: Keeps repository clean, build artifacts should not be committed

### 3. Bidi Control Character Check
- **Blocks**: Unicode bidi control characters (U+202A..U+202E, U+2066..U+2069)
- **Why**: Security - prevents code injection attacks

### 4. Required Status Checks
- Build must pass on: ubuntu-latest, macos-latest, windows-latest
- All governance checks must pass
- PR cannot be merged if any check fails

## Manual Requirements

### Before Opening PR
1. Sync with latest main: `git pull --rebase origin main`
2. Run local checks: `./scripts/verify_local.sh`
3. Ensure no emojis in commit messages
4. Update `.local` submodule if governance/rules changed

### PR Hygiene
- Use descriptive titles without emojis
- Keep diffs small (< 300 lines unless approved)
- No build artifacts committed
- All CI checks must be green before merge

## Branch Protection

Main branch requires:
- ✅ CI checks passing (build-test on all platforms)
- ✅ Governance checks passing
- ✅ No merge conflicts
- ✅ Up-to-date with main (rebase preferred)

## Violation Handling

If governance fails:
1. Fix the issue (remove emojis, delete artifacts, etc.)
2. Rebase on latest main if needed
3. Push changes - CI will re-run automatically
4. Do NOT force merge with failing checks
