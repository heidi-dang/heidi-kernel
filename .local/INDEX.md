# Project Onboarding Index

This folder is the single source of truth for:
- rules / constraints
- tasks / TODO
- context memory
- dev keywords (skillsets)

## Mandatory: read order (new developers)

1) `./.local/CONTEXT_MEMORY.md`
2) `./.local/RULES.md`
3) `./.local/DEV_SKILLSETS.md`
4) `./.local/WORKLOG_RULE.md`
5) `./.local/TODO.md`
6) `./.local/tasks/`
7) `./CONTRIBUTING.md`

## Mandatory: confirmation

Before starting any work, create your acknowledgement file:

- `./.local/ack/<your_name>.md`

Use the template in `./.local/ack/_template.md`.

A PR will not be accepted without this acknowledgement.

## Worklog (every PR)

Every contributor must add a dated entry to their worklog:

- `./.local/worklog/<your_name>.md`

Use the template in `./.local/worklog/_template.md`.

CI will fail if no worklog entry is added in a PR.
