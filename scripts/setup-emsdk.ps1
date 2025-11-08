# ==============================================================================
# DuckStation Web Build - Emscripten SDK Setup Script (Windows PowerShell)
# ==============================================================================
# This script downloads and configures the Emscripten SDK locally within the
# repository. No global installation required.
#
# Usage: .\scripts\setup-emsdk.ps1
#
# The SDK will be installed to: <REPO>\.emsdk\
# ==============================================================================

$ErrorActionPreference = "Stop"

# Determine the repository root
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$RepoRoot = Split-Path -Parent $ScriptDir
$EmsdkDir = Join-Path $RepoRoot ".emsdk"

Write-Host "=========================================="
Write-Host " DuckStation - Emscripten SDK Setup"
Write-Host "=========================================="
Write-Host ""
Write-Host "Repository root: $RepoRoot"
Write-Host "Installing emsdk to: $EmsdkDir"
Write-Host ""

# Clone emsdk if it doesn't exist
if (-Not (Test-Path $EmsdkDir)) {
    Write-Host "[1/4] Cloning Emscripten SDK..."
    git clone https://github.com/emscripten-core/emsdk.git $EmsdkDir
} else {
    Write-Host "[1/4] Emscripten SDK already cloned. Updating..."
    Push-Location $EmsdkDir
    git pull
    Pop-Location
}

# Install latest SDK
Write-Host ""
Write-Host "[2/4] Installing latest Emscripten SDK..."
Push-Location $EmsdkDir
& .\emsdk.bat install latest

# Activate latest SDK
Write-Host ""
Write-Host "[3/4] Activating latest Emscripten SDK..."
& .\emsdk.bat activate latest

Pop-Location

Write-Host ""
Write-Host "[4/4] Setup complete!"
Write-Host ""
Write-Host "=========================================="
Write-Host " IMPORTANT: Next Steps"
Write-Host "=========================================="
Write-Host ""
Write-Host "Before building, you MUST activate the emsdk environment:"
Write-Host ""
Write-Host "  Option 1 - PowerShell (this session only):"
Write-Host "    cd $EmsdkDir"
Write-Host "    .\emsdk_env.ps1"
Write-Host ""
Write-Host "  Option 2 - Command Prompt:"
Write-Host "    cd $EmsdkDir"
Write-Host "    emsdk_env.bat"
Write-Host ""
Write-Host "Then run the build script:"
Write-Host ""
Write-Host "  .\scripts\build-web.ps1"
Write-Host ""
Write-Host "=========================================="
