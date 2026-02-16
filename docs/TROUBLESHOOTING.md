# Troubleshooting Guide

This guide covers common issues developers encounter with the Heidi Kernel project setup.

## Submodule Issues

### Submodule Authentication Failed

**Symptoms:**
```
fatal: could not read Username for 'https://github.com': No such device or address
```

**Solutions:**
1. **Use SSH (recommended):**
   ```bash
   git remote set-url origin git@github.com:heidi-dang/heidi-kernel.git
   git submodule sync --recursive
   git submodule update --init --recursive
   ```

2. **Use GitHub CLI:**
   ```bash
   gh auth login
   gh auth status
   ```

3. **Check SSH keys:**
   ```bash
   ssh -T git@github.com
   # Should show: Hi <username>! You've successfully authenticated
   ```

### Submodule Not Initialized

**Symptoms:**
```
fatal: not a git repository: .local
```

**Solution:**
```bash
git submodule update --init --recursive
```

### Submodule Points to Wrong Commit

**Symptoms:**
- `.local/` contains unexpected or outdated files
- Worklog entries are missing

**Solution:**
```bash
git submodule update --remote .local
git add .local
git commit -m "chore(local): bump submodule pointer"
```

## Permission Issues

### Push Permission Denied (403)

**Symptoms:**
```
remote: Permission to heidi-dang/heidi-kernel.git denied to <username>
fatal: unable to access 'https://github.com/...': The requested URL returned error: 403
```

**Solutions:**
1. **Request collaborator access** from repo owner
2. **Check you're using the correct account:**
   ```bash
   gh auth status
   ```
3. **Use SSH if HTTPS fails:**
   ```bash
   git remote set-url origin git@github.com:heidi-dang/heidi-kernel.git
   ```

### Submodule Access Denied

**Symptoms:**
```
fatal: could not read Username for 'https://github.com': No such device or address
```

**Solutions:**
1. **Ensure you have access to private submodule repo**
2. **Use SSH authentication**
3. **Contact repo owner for submodule access**

## CI/CD Issues

### GitHub Actions Not Running

**Symptoms:**
- Push succeeded but no Actions started
- Workflow files not detected

**Solutions:**
1. **Check workflow file syntax:**
   ```bash
   ls -la .github/workflows/
   cat .github/workflows/ci.yml
   ```

2. **Trigger manually:**
   - Go to Actions tab in GitHub
   - Select workflow
   - Click "Run workflow"

3. **Check branch permissions:**
   - Ensure branch allows Actions
   - Check if workflows are disabled in repo settings

### CI Checks Failing

**Common Failures:**

1. **Submodule check failed:**
   ```bash
   git submodule status
   # Ensure .local is a valid submodule
   ```

2. **Format check failed:**
   ```bash
   ./scripts/lint.sh
   # Fix formatting issues
   ```

3. **Build failed:**
   ```bash
   cmake --preset debug
   cmake --build --preset debug
   # Check compiler and dependencies
   ```

## Environment Issues

### Wrong Git Version

**Symptoms:**
```
fatal: --recurse-submodules is not a git command
```

**Solution:**
```bash
git --version
# Upgrade to Git 2.30+ if older
```

### CMake Not Found

**Symptoms:**
```
bash: cmake: command not found
```

**Solution:**
```bash
# Ubuntu/Debian:
sudo apt-get update && sudo apt-get install -y cmake

# Or use cmake.org binary distribution
```

### Compiler Not C++23 Compatible

**Symptoms:**
```
error: 'C++23' was not recognized
```

**Solution:**
```bash
g++ --version
# Upgrade to GCC 11+ or Clang 13+
```

## Validation Commands

Run these to diagnose issues:

### Check Repository Health
```bash
# Verify .local is submodule
git ls-tree HEAD .local
# Expected: 160000 commit <hash>	.local

# Check for .local file leakage
git log -- .local
# Expected: Only submodule commits, no file contents

# Verify submodule status
git submodule status
# Expected: Clean status or specific commit hash
```

### Check Authentication
```bash
# GitHub auth status
gh auth status

# SSH connectivity
ssh -T git@github.com

# Remote URLs
git remote -v
```

### Check Build Environment
```bash
# CMake version
cmake --version

# Compiler version
g++ --version
clang++ --version

# Build presets
cmake --preset debug
```

## Recovery Procedures

### Corrupted Submodule

**Symptoms:**
- `.local/` directory exists but is not a proper git repository
- Git commands fail in `.local/` directory

**Recovery:**
```bash
rm -rf .local
git submodule update --init --recursive
```

### Detached HEAD in Submodule

**Symptoms:**
- Submodule shows "detached HEAD" state
- Cannot push submodule changes

**Recovery:**
```bash
cd .local
git checkout main
cd ..
git add .local
git commit -m "chore(local): fix submodule pointer"
```

### History Rewrite Recovery

**If you need to re-clone after history rewrite:**
```bash
# Backup any local changes
cp -r heidi-kernel heidi-kernel-backup

# Fresh clone
rm -rf heidi-kernel
git clone --recurse-submodules https://github.com/heidi-dang/heidi-kernel.git
cd heidi-kernel

# Restore any local work from backup if needed
```

## Getting Additional Help

1. **Check onboarding guide**: `docs/ONBOARDING.md`
2. **Review private resources**: `.local/INDEX.md`
3. **Search existing issues**: GitHub Issues tab
4. **Contact team**: Use project communication channels

## Prevention Tips

1. **Always run submodule update** before starting work
2. **Commit submodule pointer changes** with every PR
3. **Use SSH authentication** for more reliable access
4. **Keep GitHub CLI authenticated** for easier operations
5. **Run validation commands** regularly to catch issues early
