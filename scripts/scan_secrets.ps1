# Secret scan - checks for leaked tokens/keys in tracked files (Windows)
# Exits 0 if no secrets found, exits 1 if secrets detected

$ErrorActionPreference = "Stop"

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$RootDir = Split-Path -Parent $ScriptDir

Set-Location $RootDir

Write-Host "=== Scanning for secrets ==="

# Common secret patterns (best-effort)
$Patterns = @(
    "ghp_[a-zA-Z0-9]{36}",
    "github_pat_[a-zA-Z0-9_]{22,}",
    "[a-zA-Z0-9_-]*_TOKEN",
    "[a-zA-Z0-9_-]*_KEY",
    "AKIA[0-9A-Z]{16}",
    "sk-[a-zA-Z0-9]{20,}"
)

$Found = $false

foreach ($pattern in $Patterns) {
    $result = git grep -l -E $pattern 2>$null
    if ($result) {
        Write-Host "FAIL: Potential secret pattern '$pattern' found in:"
        Write-Host $result
        $Found = $true
    }
}

if ($Found) {
    Write-Host ""
    Write-Host "FAIL: Secrets detected. Review and remove them before committing."
    exit 1
}

Write-Host "PASS: No obvious secrets detected"
exit 0
