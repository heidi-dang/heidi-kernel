keywords: docs_only, ci_gates, lock_scope
onboarding: acknowledged
acceptance:
  - CI enforces worklog update per PR
  - CONTRIBUTING + PR template require .local onboarding

## Description

Set up governance baseline with single source of truth under `.local/`.

## Implementation

- Created `.local/INDEX.md` - onboarding index
- Created `.local/CONTEXT_MEMORY.md` - project decisions
- Created `.local/RULES.md` - non-negotiables
- Created `.local/DEV_SKILLSETS.md` - keyword workflow
- Created `.local/WORKLOG_RULE.md` - worklog requirement
- Created `.local/TODO.md` - shared TODO
- Created `.local/ack/_template.md` - acknowledgement template
- Created `.local/worklog/_template.md` - worklog template
- Updated CONTRIBUTING.md with mandatory onboarding + worklog rule
- Updated `.github/pull_request_template.md` with onboarding + worklog checkboxes
- Added `scripts/governance-gate.sh` - CI gate for INDEX.md + worklog

## Testing

- Verified all files exist
- Verified CI workflow includes governance gate
