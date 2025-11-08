# ==============================================================================
# DuckStation Web Build Script (Windows PowerShell)
# ==============================================================================
# Builds DuckStation for WebAssembly using Emscripten.
#
# Prerequisites:
#   1. Run scripts\setup-emsdk.ps1 first
#   2. Activate emsdk: cd .emsdk; .\emsdk_env.ps1
#
# Usage: .\scripts\build-web.ps1 [BuildType]
#   BuildType: RelWithDebInfo (default), Release, or Debug
# ==============================================================================

param(
    [string]$BuildType = "RelWithDebInfo"
)

$ErrorActionPreference = "Stop"

# Configuration
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$RepoRoot = Split-Path -Parent $ScriptDir
$BuildDir = Join-Path $RepoRoot "build-web"
$EmsdkDir = Join-Path $RepoRoot ".emsdk"

Write-Host "=========================================="
Write-Host " DuckStation - Web Build"
Write-Host "=========================================="
Write-Host ""
Write-Host "Repository: $RepoRoot"
Write-Host "Build directory: $BuildDir"
Write-Host "Build type: $BuildType"
Write-Host ""

# Check if emsdk exists
if (-Not (Test-Path $EmsdkDir)) {
    Write-Host "ERROR: Emscripten SDK not found at $EmsdkDir" -ForegroundColor Red
    Write-Host ""
    Write-Host "Please run the setup script first:"
    Write-Host "  .\scripts\setup-emsdk.ps1"
    exit 1
}

# Activate emsdk environment
Write-Host "[Step 0/3] Activating Emscripten environment..."
Write-Host ""

# Set EMSDK environment variable
$env:EMSDK = $EmsdkDir.Replace('\', '/')

# Manually add emsdk paths to PATH
$env:PATH = "$EmsdkDir;$EmsdkDir\upstream\emscripten;$env:PATH"

# Set Python and Node paths
$PythonDir = Join-Path $EmsdkDir "python\3.13.3_64bit"
$NodeDir = Join-Path $EmsdkDir "node\22.16.0_64bit\bin"

if (Test-Path $PythonDir) {
    $env:EMSDK_PYTHON = Join-Path $PythonDir "python.exe"
}

if (Test-Path $NodeDir) {
    $env:EMSDK_NODE = Join-Path $NodeDir "node.exe"
}

Write-Host "EMSDK environment configured:"
Write-Host "  EMSDK = $env:EMSDK"
Write-Host "  EMSDK_PYTHON = $env:EMSDK_PYTHON"
Write-Host "  EMSDK_NODE = $env:EMSDK_NODE"
Write-Host ""

# Check if emcc is available
$emccPath = Join-Path $EmsdkDir "upstream\emscripten\emcc.bat"
if (-Not (Test-Path $emccPath)) {
    Write-Host "ERROR: Emscripten compiler (emcc.bat) not found at $emccPath!" -ForegroundColor Red
    Write-Host ""
    Write-Host "The emsdk installation appears incomplete."
    Write-Host "Please try running emsdk setup again:"
    Write-Host "  .\scripts\setup-emsdk.ps1"
    exit 1
}

# Test emcc - call emcc.bat directly and suppress stderr
# Emscripten writes informational messages to stderr which PowerShell treats as errors
$emccBat = Join-Path $EmsdkDir "upstream\emscripten\emcc.bat"

# Run emcc.bat with stderr redirected to null to suppress "Running sanity checks" messages
$emccOutput = cmd /c "`"$emccBat`" --version 2>nul"
$emccExitCode = $LASTEXITCODE

if ($emccExitCode -eq 0) {
    # Extract version from output (first line that contains "emcc")
    $versionLine = $emccOutput | Where-Object { $_ -match "emcc" } | Select-Object -First 1
    if ($versionLine) {
        Write-Host "Emscripten compiler found: $versionLine"
    } else {
        Write-Host "Emscripten compiler found and working"
    }
    Write-Host ""
} else {
    Write-Host "ERROR: Emscripten compiler (emcc) failed to run!" -ForegroundColor Red
    Write-Host ""
    Write-Host "Exit code: $emccExitCode"
    if ($emccOutput) {
        Write-Host "Output:"
        $emccOutput | ForEach-Object { Write-Host "  $_" }
    }
    Write-Host ""
    Write-Host "The emsdk environment couldn't be activated properly."
    Write-Host "Please try running emsdk setup again:"
    Write-Host "  .\scripts\setup-emsdk.ps1"
    exit 1
}

# Create build directory
if (-Not (Test-Path $BuildDir)) {
    New-Item -ItemType Directory -Path $BuildDir | Out-Null
}
Push-Location $BuildDir

Write-Host "[1/4] Configuring CMake..."
Write-Host ""

# CMake configuration with Emscripten
$cmakeArgs = @(
    "-S", $RepoRoot,
    "-B", ".",
    "-G", "Ninja",
    "-DCMAKE_BUILD_TYPE=$BuildType",
    "-DCMAKE_INTERPROCEDURAL_OPTIMIZATION=OFF",
    "-DCMAKE_TOOLCHAIN_FILE=$env:EMSDK/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake",
    "-DCMAKE_C_FLAGS=-fno-exceptions",
    "-DCMAKE_CXX_FLAGS=-fno-exceptions -fno-rtti",
    "-DENABLE_OPENGL=ON",
    "-DENABLE_VULKAN=OFF",
    "-DBUILD_QT_FRONTEND=OFF",
    "-DBUILD_MINI_FRONTEND=OFF",
    "-DBUILD_REGTEST=OFF",
    "-DBUILD_TESTS=OFF",
    "-DENABLE_X11=OFF",
    "-DENABLE_WAYLAND=OFF"
)

& cmake @cmakeArgs
if ($LASTEXITCODE -ne 0) {
    Write-Host ""
    Write-Host "ERROR: CMake configuration failed!" -ForegroundColor Red
    Write-Host ""
    Write-Host "Common issues:"
    Write-Host "  - Missing dependencies (check dep/ folder)"
    Write-Host "  - Incompatible CMake options for Emscripten"
    Write-Host "  - Missing Ninja build tool"
    Write-Host ""
    Write-Host "Check the output above for specific errors."
    Pop-Location
    exit 1
}

Write-Host ""
Write-Host "[2/4] Building..."
Write-Host ""

# Build
$numCores = (Get-CimInstance Win32_ComputerSystem).NumberOfLogicalProcessors
& cmake --build . --parallel $numCores
if ($LASTEXITCODE -ne 0) {
    Write-Host ""
    Write-Host "ERROR: Build failed!" -ForegroundColor Red
    Write-Host ""
    Write-Host "Check compiler output above for errors."
    Write-Host "You may need to fix source code issues or adjust CMake flags."
    Pop-Location
    exit 1
}

Write-Host ""
Write-Host "[3/4] Copying artifacts to web-dist..."
Write-Host ""

# Create output directory
$WebDist = Join-Path $RepoRoot "web-dist\duckstation"
if (-Not (Test-Path $WebDist)) {
    New-Item -ItemType Directory -Path $WebDist -Force | Out-Null
}

# Copy WASM artifacts
$BinDir = Join-Path $BuildDir "bin"

if (Test-Path (Join-Path $BinDir "duckstation-web.js")) {
    Copy-Item (Join-Path $BinDir "duckstation-web.js") $WebDist
    Write-Host "✓ Copied duckstation-web.js"
}

if (Test-Path (Join-Path $BinDir "duckstation-web.wasm")) {
    $wasmFile = Get-Item (Join-Path $BinDir "duckstation-web.wasm")
    $wasmSize = "{0:N2} MB" -f ($wasmFile.Length / 1MB)
    Copy-Item $wasmFile.FullName $WebDist
    Write-Host "✓ Copied duckstation-web.wasm ($wasmSize)"
}

# Copy worker if it exists
if (Test-Path (Join-Path $BinDir "duckstation-web.worker.js")) {
    Copy-Item (Join-Path $BinDir "duckstation-web.worker.js") $WebDist
    Write-Host "✓ Copied duckstation-web.worker.js"
}

# Copy data files if they exist
if (Test-Path (Join-Path $BinDir "duckstation-web.data")) {
    $dataFile = Get-Item (Join-Path $BinDir "duckstation-web.data")
    $dataSize = "{0:N2} MB" -f ($dataFile.Length / 1MB)
    Copy-Item $dataFile.FullName $WebDist
    Write-Host "✓ Copied duckstation-web.data ($dataSize)"
}

Pop-Location

Write-Host ""
Write-Host "=========================================="
Write-Host " Build Complete!"
Write-Host "=========================================="
Write-Host ""
Write-Host "Artifacts location: $WebDist"
Write-Host ""
Write-Host "Files generated:"
Get-ChildItem $WebDist | ForEach-Object {
    $size = "{0:N2} MB" -f ($_.Length / 1MB)
    Write-Host "  $($_.Name) ($size)"
}
Write-Host ""
Write-Host "Next steps:"
Write-Host "  1. Start the development server:"
Write-Host "       .\scripts\serve-web.ps1"
Write-Host "  2. Open http://localhost:8080 in your browser"
Write-Host ""
Write-Host "=========================================="
