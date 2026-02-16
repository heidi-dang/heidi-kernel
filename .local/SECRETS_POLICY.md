# Secrets Policy (Strict)

## Rule
Do not store real secrets in any git repo (public or private). This includes tokens, passwords, SSH private keys, refresh tokens, cookies, or API keys.

## Allowed
- Templates with placeholders (no real values)
- Instructions on how to create/store secrets locally

## Required local storage
Each developer stores their own GitHub token locally with file permissions 0600.
Recommended location:
- `~/.config/heidi/secrets/github_token`

## Recovery
If credentials are missing, re-authenticate locally. Never paste secrets into PRs, issues, or chat.
