# Branch Protection Recommendations

These settings are recommended for the `main` branch to ensure stability and governance compliance.

## Required Settings

### Require pull request reviews before merging
- Required approving reviews: 1
- Dismiss stale reviews when new commits are pushed
- Require review from code owners (optional)

### Require status checks to pass before merging
- Require branches to be up to date before merging
- Select required checks:
  - `ci` (GitHub Actions)

### Require conversation resolution before merging
- All review threads must be resolved

### Include administrators
- Apply to administrators (ensures consistent enforcement)

## Recommended Settings

### Require signed commits
- Not required for this project (simpler workflow)

### Require linear history
- Recommended after initial cleanup stabilizes
- Prevents merge commits from cluttering history

### Allow force pushes
- **Disallow** force pushes to main
- Exception: only repository admin can force push during emergency recovery

### Allow deletions
- **Disallow** branch deletion

## Implementation

Go to Repository Settings > Branches > Add rule:

1. Branch name pattern: `main`
2. Check: "Require pull request reviews before merging"
3. Check: "Require status checks to pass before merging"
4. Check: "Require conversation resolution before merging"
5. Check: "Include administrators"
6. Check: "Require linear history" (after stabilization)
7. Check: "Allow force pushes" -> select "Administrators only" or "Never"

## Review Checklist

- [ ] PR reviews required
- [ ] CI check passes
- [ ] Branch up to date
- [ ] No unresolved comments
- [ ] No force pushes
