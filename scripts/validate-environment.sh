#!/bin/bash

# Environment Validation Script for Heidi Kernel
# This script validates that the development environment is properly set up

set -euo pipefail

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Counter for issues found
ISSUES=0

# Function to print status
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

# Function to check command exists
command_exists() {
    command -v "$1" >/dev/null 2>&1
}

# Function to check version constraint
check_version() {
    local cmd=$1
    local min_version=$2
    local current_version=$3
    
    if command_exists "$cmd"; then
        if printf '%s\n%s\n' "$min_version" "$current_version" | sort -V -C; then
            return 0
        else
            return 1
        fi
    else
        return 1
    fi
}

echo "Heidi Kernel Environment Validation"
echo "=================================="
echo

# Check Git
echo "Checking Git..."
if command_exists git; then
    GIT_VERSION=$(git --version | cut -d' ' -f3)
    if check_version git "2.30" "$GIT_VERSION"; then
        print_status "OK" "Git $GIT_VERSION (supports submodules)"
    else
        print_status "WARN" "Git $GIT_VERSION (upgrade to 2.30+ recommended for submodule support)"
    fi
else
    print_status "FAIL" "Git not found"
fi

# Check CMake
echo
echo "Checking CMake..."
if command_exists cmake; then
    CMAKE_VERSION=$(cmake --version | head -n1 | cut -d' ' -f3)
    if check_version cmake "3.20" "$CMAKE_VERSION"; then
        print_status "OK" "CMake $CMAKE_VERSION"
    else
        print_status "FAIL" "CMake $CMAKE_VERSION (need 3.20+)"
    fi
else
    print_status "FAIL" "CMake not found"
fi

# Check C++ Compiler
echo
echo "Checking C++ Compiler..."
if command_exists g++; then
    GCC_VERSION=$(g++ --version | head -n1 | grep -oP '\d+\.\d+')
    if check_version g++ "11.0" "$GCC_VERSION"; then
        print_status "OK" "g++ $GCC_VERSION (C++23 support)"
    else
        print_status "WARN" "g++ $GCC_VERSION (C++23 support may be limited)"
    fi
else
    print_status "FAIL" "g++ not found"
fi

if command_exists clang++; then
    CLANG_VERSION=$(clang++ --version | head -n1 | grep -oP '\d+\.\d+')
    if check_version clang++ "13.0" "$CLANG_VERSION"; then
        print_status "OK" "clang++ $CLANG_VERSION (C++23 support)"
    else
        print_status "WARN" "clang++ $CLANG_VERSION (C++23 support may be limited)"
    fi
fi

# Check GitHub CLI
echo
echo "Checking GitHub CLI..."
if command_exists gh; then
    if gh auth status >/dev/null 2>&1; then
        print_status "OK" "GitHub CLI authenticated"
    else
        print_status "WARN" "GitHub CLI installed but not authenticated (run 'gh auth login')"
    fi
else
    print_status "WARN" "GitHub CLI not found (recommended for authentication)"
fi

# Check Repository Structure
echo
echo "Checking Repository Structure..."

# Check if we're in git repo
if git rev-parse --git-dir >/dev/null 2>&1; then
    print_status "OK" "In Git repository"
else
    print_status "FAIL" "Not in a Git repository"
fi

# Check .local submodule
if [ -d ".local" ]; then
    if git ls-tree HEAD .local | grep -q "160000 commit"; then
        print_status "OK" ".local is a submodule"
        
        # Check if submodule is initialized
        if [ -f ".local/.git" ] || git rev-parse --resolve-git-dir .local/.git >/dev/null 2>&1; then
            print_status "OK" ".local submodule is initialized"
        else
            print_status "WARN" ".local submodule not initialized (run 'git submodule update --init --recursive')"
        fi
    else
        print_status "FAIL" ".local exists but is not a submodule"
    fi
else
    print_status "FAIL" ".local directory not found"
fi

# Check for .local file leakage in history
echo
echo "Checking for .local file leakage..."
if git log -- .local 2>/dev/null | grep -q "^\+[[:space:]]*[^[:space:]]"; then
    print_status "FAIL" ".local files found in git history (should only be submodule commits)"
else
    print_status "OK" "No .local file leakage in history"
fi

# Check essential files
echo
echo "Checking Essential Files..."
essential_files=("LICENSE" "README.md" "CONTRIBUTING.md" "CMakePresets.json")
for file in "${essential_files[@]}"; do
    if [ -f "$file" ]; then
        print_status "OK" "$file exists"
    else
        print_status "WARN" "$file not found"
    fi
done

# Check documentation
if [ -d "docs" ]; then
    doc_files=("docs/ONBOARDING.md" "docs/TROUBLESHOOTING.md" "docs/ARCHITECTURE.md")
    for file in "${doc_files[@]}"; do
        if [ -f "$file" ]; then
            print_status "OK" "$file exists"
        else
            print_status "WARN" "$file not found"
        fi
    done
else
    print_status "WARN" "docs directory not found"
fi

# Check build presets
echo
echo "Checking Build Presets..."
if [ -f "CMakePresets.json" ]; then
    if grep -q "debug" CMakePresets.json; then
        print_status "OK" "Debug preset found"
    else
        print_status "WARN" "Debug preset not found in CMakePresets.json"
    fi
else
    print_status "FAIL" "CMakePresets.json not found"
fi

# Summary
echo
echo "Validation Summary"
echo "=================="
if [ $ISSUES -eq 0 ]; then
    echo -e "${GREEN}All checks passed!${NC} Your environment is ready for development."
    exit 0
else
    echo -e "${RED}Found $ISSUES issue(s) that need attention.${NC}"
    echo
    echo "Please address the issues above before proceeding with development."
    echo "See docs/TROUBLESHOOTING.md for help with common problems."
    exit 1
fi
