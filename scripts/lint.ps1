# Lint script - runs clang-tidy on C++ source files (Windows)
# Requires compile_commands.json from a debug build

$ErrorActionPreference = "Continue"

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$RootDir = Split-Path -Parent $ScriptDir

Set-Location $RootDir

Write-Host "=== Running lint checks ==="

# Check for clang-tidy
$clangTidy = Get-Command clang-tidy -ErrorAction SilentlyContinue
if (-not $clangTidy) {
    Write-Host "WARN: clang-tidy not found; skipping" 
    exit 0
}

# Check for compile_commands.json
$CC_DB = Join-Path $RootDir "build\debug\compile_commands.json"
if (-not (Test-Path $CC_DB)) {
    Write-Host "WARN: compile_commands.json not found (build debug first); skipping clang-tidy"
    Write-Host "Run: cmake --preset debug"
    exit 0
}

# Find C++ source files
$CppFiles = git ls-files "*.cpp" "*.cc" "*.cxx" 2>$null

if ($CppFiles.Count -eq 0) {
    Write-Host "No C++ source files found."
    exit 0
}

# Run clang-tidy on a bounded set of files
$Errors = 0
$Count = 0

foreach ($tu in $CppFiles) {
    if ($Count -ge 50) { break }
    Write-Host "Checking: $tu"
    $result = clang-tidy $tu -p "$RootDir\build\debug" 2>&1
    if ($LASTEXITCODE -ne 0) {
        $Errors = 1
    }
    $Count++
}

if ($Errors -eq 1) {
    Write-Host "FAIL: clang-tidy found issues"
    exit 1
}

Write-Host "PASS: lint check complete"
exit 0
