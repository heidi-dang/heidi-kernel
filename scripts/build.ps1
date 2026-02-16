# heidi-kernel build script for Windows

$ErrorActionPreference = "Stop"

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $ScriptDir

Write-Host "=== Building heidi-kernel (Debug) ==="
cmake --preset debug
cmake --build --preset debug

Write-Host "=== Build complete ==="
