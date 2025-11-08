# ==============================================================================
# DuckStation Web Build - Complete Build Script (Windows PowerShell)
# ==============================================================================
# This script runs setup + build in one command.
# Perfect for first-time builds or when you want to ensure everything is fresh.
#
# Usage: .\scripts\build-web-complete.ps1 [BuildType]
#   BuildType: RelWithDebInfo (default), Release, or Debug
# ==============================================================================

param(
    [string]$BuildType = "RelWithDebInfo"
)

$ErrorActionPreference = "Stop"

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$RepoRoot = Split-Path -Parent $ScriptDir

Write-Host "=========================================="
Write-Host " DuckStation - Complete Web Build"
Write-Host "=========================================="
Write-Host ""
Write-Host "This will:"
Write-Host "  1. Setup/update Emscripten SDK"
Write-Host "  2. Build DuckStation for WebAssembly"
Write-Host "  3. Copy artifacts to web-dist/"
Write-Host ""
Write-Host "Build type: $BuildType"
Write-Host ""
Write-Host "=========================================="
Write-Host ""

# Step 1: Setup emsdk
Write-Host "[STEP 1] Setting up Emscripten SDK..."
Write-Host ""
$setupScript = Join-Path $ScriptDir "setup-emsdk.ps1"
& $setupScript
if ($LASTEXITCODE -ne 0) {
    Write-Host ""
    Write-Host "ERROR: Setup failed!" -ForegroundColor Red
    exit 1
}

Write-Host ""
Write-Host "=========================================="
Write-Host ""

# Step 2: Build
Write-Host "[STEP 2] Building DuckStation Web..."
Write-Host ""
$buildScript = Join-Path $ScriptDir "build-web.ps1"
& $buildScript -BuildType $BuildType
if ($LASTEXITCODE -ne 0) {
    Write-Host ""
    Write-Host "ERROR: Build failed!" -ForegroundColor Red
    exit 1
}

Write-Host ""
Write-Host "=========================================="
Write-Host " Complete Build Finished!"
Write-Host "=========================================="
Write-Host ""
Write-Host "You can now start the dev server:"
Write-Host "  .\scripts\serve-web.ps1"
Write-Host ""
Write-Host "Then open: http://localhost:8080"
Write-Host ""
Write-Host "=========================================="
