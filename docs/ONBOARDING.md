# Onboarding Guide

## Welcome to Heidi Kernel

This guide will get you set up with the Heidi Kernel project in your first 15 minutes.

## Repository Layout

```
heidi-kernel/
├── .local/          # Private submodule (developer resources)
├── docs/            # Public documentation
├── scripts/         # Developer helper scripts
├── .github/         # GitHub templates and workflows
├── CMakePresets.json
├── CONTRIBUTING.md
└── README.md
```

## The .local Submodule (Important)

`.local` is a **private submodule** that contains developer-specific resources:
- Project rules and constraints
- Development skillsets and workflows
- Worklog templates and policies
- Private governance documents

**Critical Rule**: Never copy `.local` contents into the public repo. All project instructions live in `.local` but must remain private.

## First 15 Minutes Checklist

### 1. Clone with Submodules
```bash
git clone --recurse-submodules https://github.com/heidi-dang/heidi-kernel.git
cd heidi-kernel
```

### 2. Verify Submodule Status
```bash
git status
git submodule status
```

Expected output: Clean status, `.local` shows as submodule

### 3. Read Private Resources
Navigate to `.local/` and read in this order:
1. `INDEX.md` - Overview of all resources
2. `RULES.md` - Non-negotiable kernel constraints
3. `DEV_SKILLSETS.md` - Development keywords and task requirements
4. `WORKLOG_RULE.md` - Worklog requirements per PR
5. `SECRETS_POLICY.md` - Security policy (no tokens in repo)

### 4. Create Your Acknowledgement
```bash
cp .local/ack/_template.md .local/ack/<your_name>.md
# Edit the file with your information and signature
```

### 5. Set Up Your Worklog
```bash
cp .local/worklog/_template.md .local/worklog/<your_name>.md
```

### 6. Run Environment Validation
```bash
./scripts/validate-environment.sh  # if available
```

## Required Tools

- Git 2.30+ (for submodule support)
- CMake 3.20+
- C++23 compatible compiler
- GitHub CLI (recommended for authentication)

## Common First Tasks

1. **Read the Architecture**: `docs/ARCHITECTURE.md`
2. **Understand Constraints**: `.local/RULES.md`
3. **Review Process Policy**: `docs/PROCESS_POLICY.md`
4. **Set Up Development Environment**: Follow `docs/WSL2.md`

## Submodule Workflow

### Daily Sync
```bash
git checkout main
git pull --rebase origin main
git submodule update --init --recursive
```

### Start Feature Branch
```bash
git checkout -b feature/<short-name>
git submodule update --init --recursive
```

### Update Private Resources
1. Make changes in `.local/`
2. Commit and push to private submodule repo
3. Update submodule pointer in public repo:
   ```bash
   git add .local
   git commit -m "chore(local): bump submodule pointer"
   ```

## Security Rules

- Never store tokens, passwords, or API keys in any git repo
- Use local credential storage (0600 permissions) or GitHub CLI auth
- Never paste secrets into PRs, issues, or chat
- See `.local/SECRETS_POLICY.md` for details

## Getting Help

1. Check `docs/TROUBLESHOOTING.md` for common issues
2. Review `.local/` resources for detailed procedures
3. Contact the team through project channels

## Validation Commands

Run these to verify your setup:
```bash
# Verify .local is submodule (not files)
git ls-tree HEAD .local
# Should show: 160000 commit <hash>	.local

# Verify no .local files in public history
git log -- .local
# Should show only submodule commits, no file contents

# Check submodule status
git submodule status
# Should show clean status
```

## Next Steps

After completing this checklist:
1. Read the development skillsets in `.local/DEV_SKILLSETS.md`
2. Review the contribution guidelines in `CONTRIBUTING.md`
3. Start with a small, well-defined task
4. Always update your worklog per PR

Welcome to the team!
