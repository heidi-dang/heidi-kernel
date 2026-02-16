#!/bin/bash

# Pre-commit Validation Script
# Runs before committing to prevent common issues

set -euo pipefail

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

ISSUES=0

print_status() {
    local status=$1
    local message=$2
    
    case $status in
        "OK")
            echo -e "${GREEN}✓${NC} $message"
            ;;
        "WARN")
            echo -e "${YELLOW}⚠${NC} $message"
            ;;
        "FAIL")
            echo -e "${RED}✗${NC} $message"
            ((ISSUES++))
            ;;
    esac
}

echo "Pre-commit Validation"
echo "===================="
echo

# Check if we're committing .local files (should not happen)
echo "Checking for .local file changes..."
if git diff --cached --name-only | grep -E '^\.local/' | grep -v '^\.local$'; then
    print_status "FAIL" "Found .local file changes in staging area"
    echo "  .local files should only be modified in the private submodule repo"
    echo "  Remove these files from staging: git reset HEAD .local/<files>"
else
    print_status "OK" "No .local file changes in staging area"
fi

# Check if .local submodule pointer is being updated
echo
echo "Checking .local submodule..."
if git diff --cached --name-only | grep -E '^\.local$'; then
    print_status "OK" ".local submodule pointer is being updated"
    
    # Verify the submodule is valid
    if git ls-tree HEAD .local | grep -q "160000 commit"; then
        print_status "OK" ".local is a valid submodule"
    else
        print_status "FAIL" ".local is not a valid submodule"
    fi
else
    print_status "WARN" ".local submodule pointer not updated (may be intentional)"
fi

# Check for secrets in staged files
echo
echo "Checking for potential secrets..."
SECRET_PATTERNS=(
    "password"
    "token"
    "secret"
    "api_key"
    "private_key"
    "BEGIN.*PRIVATE KEY"
    "ghp_"
    "gho_"
    "ghu_"
    "ghs_"
    "ghr_"
)

SECRETS_FOUND=false
for pattern in "${SECRET_PATTERNS[@]}"; do
    if git diff --cached --text | grep -i "$pattern" >/dev/null; then
        print_status "FAIL" "Potential secret found matching pattern: $pattern"
        SECRETS_FOUND=true
    fi
done

if ! $SECRETS_FOUND; then
    print_status "OK" "No obvious secrets detected in staged changes"
fi

# Check for large files
echo
echo "Checking for large files..."
LARGE_FILES=$(git diff --cached --name-only | xargs -I {} sh -c 'test -f "{}" && echo "{}"')
for file in $LARGE_FILES; do
    if [ -f "$file" ]; then
        size=$(stat -c%s "$file" 2>/dev/null || echo 0)
        if [ "$size" -gt 1048576 ]; then  # 1MB
            print_status "WARN" "Large file detected: $file (${size} bytes)"
        fi
    fi
done

if [ -z "$LARGE_FILES" ] || [ "$size" -le 1048576 ]; then
    print_status "OK" "No unusually large files detected"
fi

# Check file permissions for scripts
echo
echo "Checking script permissions..."
SCRIPT_FILES=$(git diff --cached --name-only | grep -E '\.(sh|py)$')
for file in $SCRIPT_FILES; do
    if [ -f "$file" ]; then
        if [ -x "$file" ]; then
            print_status "OK" "$file has execute permission"
        else
            print_status "WARN" "$file lacks execute permission"
        fi
    fi
done

if [ -z "$SCRIPT_FILES" ]; then
    print_status "OK" "No script files to check"
fi

# Check for proper commit message format (if this is being run as a hook)
if [ "${1:-}" = "hook" ] && [ -n "${2:-}" ]; then
    commit_msg="$2"
    echo
    echo "Checking commit message format..."
    
    # Check for emojis (not allowed)
    if echo "$commit_msg" | grep -E '[\U0001F600-\U0001F64F\U0001F300-\U0001F5FF\U0001F680-\U0001F6FF\U0001F1E0-\U0001F1FF]' >/dev/null; then
        print_status "FAIL" "Commit message contains emojis (not allowed)"
    else
        print_status "OK" "No emojis in commit message"
    fi
    
    # Check minimum length
    if [ ${#commit_msg} -lt 10 ]; then
        print_status "FAIL" "Commit message too short (minimum 10 characters)"
    else
        print_status "OK" "Commit message length acceptable"
    fi
fi

# Summary
echo
echo "Pre-commit Summary"
echo "=================="
if [ $ISSUES -eq 0 ]; then
    echo -e "${GREEN}Pre-commit validation passed!${NC}"
    exit 0
else
    echo -e "${RED}Pre-commit validation failed with $ISSUES issue(s).${NC}"
    echo
    echo "Please fix the issues above before committing."
    echo "Use 'git commit --no-verify' to bypass this check (not recommended)."
    exit 1
fi
