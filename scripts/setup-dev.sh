#!/bin/bash

# Development Environment Setup Script
# Sets up git hooks and validates the environment

set -euo pipefail

# Colors
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

echo "Heidi Kernel Development Setup"
echo "=============================="
echo

# Get script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(git rev-parse --show-toplevel 2>/dev/null || echo "$(dirname "$SCRIPT_DIR")")"

cd "$REPO_ROOT"

echo "Setting up development environment in: $REPO_ROOT"
echo

# Setup git hooks
echo "Setting up git hooks..."
HOOKS_DIR=".git/hooks"
mkdir -p "$HOOKS_DIR"

# Pre-commit hook
PRE_COMMIT_HOOK="$HOOKS_DIR/pre-commit"
cat > "$PRE_COMMIT_HOOK" << 'EOF'
#!/bin/bash
# Run pre-commit validation
SCRIPT_DIR="$(git rev-parse --show-toplevel)/scripts"
"$SCRIPT_DIR/pre-commit-validate.sh" hook "$1"
EOF

chmod +x "$PRE_COMMIT_HOOK"
echo -e "${GREEN}✓${NC} Pre-commit hook installed"

# Pre-push hook (optional)
PRE_PUSH_HOOK="$HOOKS_DIR/pre-push"
cat > "$PRE_PUSH_HOOK" << 'EOF'
#!/bin/bash
# Basic pre-push validation
echo "Running pre-push validation..."

# Check if .local is a valid submodule
if ! git ls-tree HEAD .local | grep -q "160000 commit"; then
    echo "ERROR: .local is not a valid submodule"
    exit 1
fi

echo "Pre-push validation passed"
EOF

chmod +x "$PRE_PUSH_HOOK"
echo -e "${GREEN}✓${NC} Pre-push hook installed"

echo
echo "Running environment validation..."
if "$SCRIPT_DIR/validate-environment.sh"; then
    echo -e "${GREEN}✓${NC} Environment validation passed"
else
    echo -e "${YELLOW}⚠${NC} Environment validation found issues (see above)"
    echo "    You may want to fix these before continuing development."
fi

echo
echo "Development setup complete!"
echo
echo "Next steps:"
echo "1. Read docs/ONBOARDING.md if you haven't already"
echo "2. Set up your worklog: cp .local/worklog/_template.md .local/worklog/<your_name>.md"
echo "3. Create your acknowledgement: cp .local/ack/_template.md .local/ack/<your_name>.md"
echo "4. Start development with: git checkout -b feature/<your-feature>"
echo
echo "The following git hooks are now active:"
echo "- Pre-commit: Validates changes before commit"
echo "- Pre-push: Basic validation before push"
echo
echo "To bypass hooks (not recommended):"
echo "- git commit --no-verify"
echo "- git push --no-verify"
