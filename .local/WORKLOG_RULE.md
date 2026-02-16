# Worklog Rule

Every contributor must maintain a personal worklog.

## Requirement

- Create file: `.local/worklog/<your_name>.md`
- Add a dated entry for **every PR**

## Entry format

```markdown
## YYYY-MM-DD: <PR title>

- <what you did>
- <what was tested>
```

## CI enforcement

PRs will fail if no file under `.local/worklog/` was modified.

## Rationale

- Provides audit trail
- Helps track who did what
- Enables retrospective analysis
