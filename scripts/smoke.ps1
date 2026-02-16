# heidi-kernel smoke test for Windows

$ErrorActionPreference = "Stop"

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $ScriptDir

Write-Host "=== Building heidi-kernel ==="
cmake --preset debug
cmake --build --preset debug

Write-Host ""
Write-Host "=== Running heidi-kernel (will stop after 2 seconds) ==="
$proc = Start-Process -FilePath "./build/debug/heidi-kernel" -NoNewWindow -PassThru
Start-Sleep -Seconds 2
Stop-Process -Id $proc.Id -Force -ErrorAction SilentlyContinue

Write-Host ""
Write-Host "=== Checking version flag ==="
& ./build/debug/heidi-kernel --version

Write-Host ""
Write-Host "=== Checking help flag ==="
& ./build/debug/heidi-kernel --help

Write-Host ""
Write-Host "=== Smoke test complete ==="
