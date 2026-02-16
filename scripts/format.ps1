# Format check - runs clang-format on all C++ source files (Windows)
# Exit 0 if files are formatted correctly, exit 1 if formatting needed

$ErrorActionPreference = "Stop"

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$RootDir = Split-Path -Parent $ScriptDir

Set-Location $RootDir

Write-Host "=== Running clang-format check ==="

# Find C++ source files
$CppFiles = Get-ChildItem -Recurse -Include "*.cpp","*.hpp","*.h","*.cc","*.cxx" | Where-Object { $_.FullName -notmatch "\\build\\|\\.git\\" }

if ($CppFiles.Count -eq 0) {
    Write-Host "No C++ source files found."
    exit 0
}

$Unformatted = $false

foreach ($file in $CppFiles) {
    $original = Get-Content $file.FullName -Raw
    $formatted = clang-format --style=file $file.FullName 2>$null
    if ($original -ne $formatted) {
        Write-Host "Formatting needed: $($file.FullName)"
        $Unformatted = $true
    }
}

if ($Unformatted) {
    Write-Host ""
    Write-Host "FAIL: Some files need formatting. Run:"
    Write-Host "  clang-format -i --style=file <files>"
    exit 1
}

Write-Host "PASS: All files formatted correctly"
exit 0
