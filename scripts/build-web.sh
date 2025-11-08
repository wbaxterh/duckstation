#!/usr/bin/env bash
# ==============================================================================
# DuckStation Web Build Script
# ==============================================================================
# Builds DuckStation for WebAssembly using Emscripten.
#
# Prerequisites:
#   1. Run scripts/setup-emsdk.sh first
#   2. Source the emsdk environment: source .emsdk/emsdk_env.sh
#
# Usage: bash scripts/build-web.sh [build-type]
#   build-type: RelWithDebInfo (default), Release, or Debug
# ==============================================================================

set -euo pipefail

# Configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
BUILD_DIR="${REPO_ROOT}/build-web"
BUILD_TYPE="${1:-RelWithDebInfo}"

echo "=========================================="
echo " DuckStation - Web Build"
echo "=========================================="
echo ""
echo "Repository: ${REPO_ROOT}"
echo "Build directory: ${BUILD_DIR}"
echo "Build type: ${BUILD_TYPE}"
echo ""

# Check if emsdk is activated
if ! command -v emcc &> /dev/null; then
  echo "ERROR: Emscripten compiler (emcc) not found!"
  echo ""
  echo "Did you source the emsdk environment?"
  echo "  source ${REPO_ROOT}/.emsdk/emsdk_env.sh"
  echo ""
  echo "If you haven't installed emsdk yet, run:"
  echo "  bash ${SCRIPT_DIR}/setup-emsdk.sh"
  exit 1
fi

echo "Emscripten compiler found: $(emcc --version | head -n1)"
echo ""

# Create build directory
mkdir -p "${BUILD_DIR}"
cd "${BUILD_DIR}"

echo "[1/3] Configuring CMake..."
echo ""

# CMake configuration with Emscripten
emcmake cmake \
  -S "${REPO_ROOT}" \
  -B . \
  -G Ninja \
  -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" \
  -DCMAKE_INTERPROCEDURAL_OPTIMIZATION=OFF \
  -DCMAKE_C_FLAGS="-fno-exceptions" \
  -DCMAKE_CXX_FLAGS="-fno-exceptions -fno-rtti" \
  -DENABLE_OPENGL=ON \
  -DENABLE_VULKAN=OFF \
  -DBUILD_QT_FRONTEND=OFF \
  -DBUILD_MINI_FRONTEND=OFF \
  -DBUILD_REGTEST=OFF \
  -DBUILD_TESTS=OFF \
  -DENABLE_X11=OFF \
  -DENABLE_WAYLAND=OFF \
  || {
    echo ""
    echo "ERROR: CMake configuration failed!"
    echo ""
    echo "Common issues:"
    echo "  - Missing dependencies (check dep/ folder)"
    echo "  - Incompatible CMake options for Emscripten"
    echo ""
    echo "Check the output above for specific errors."
    exit 1
  }

echo ""
echo "[2/3] Building..."
echo ""

# Build
cmake --build . --parallel $(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4) || {
  echo ""
  echo "ERROR: Build failed!"
  echo ""
  echo "Check compiler output above for errors."
  echo "You may need to fix source code issues or adjust CMake flags."
  exit 1
}

echo ""
echo "[3/3] Installing to web-dist..."
echo ""

# Create output directory
WEB_DIST="${REPO_ROOT}/web-dist/duckstation"
mkdir -p "${WEB_DIST}"

# Copy WASM artifacts
if [ -f "${BUILD_DIR}/bin/duckstation-web.js" ]; then
  cp "${BUILD_DIR}/bin/duckstation-web.js" "${WEB_DIST}/"
  echo "✓ Copied duckstation-web.js"
fi

if [ -f "${BUILD_DIR}/bin/duckstation-web.wasm" ]; then
  cp "${BUILD_DIR}/bin/duckstation-web.wasm" "${WEB_DIST}/"
  WASM_SIZE=$(du -h "${BUILD_DIR}/bin/duckstation-web.wasm" | cut -f1)
  echo "✓ Copied duckstation-web.wasm (${WASM_SIZE})"
fi

# Copy worker if it exists (generated with pthread support)
if [ -f "${BUILD_DIR}/bin/duckstation-web.worker.js" ]; then
  cp "${BUILD_DIR}/bin/duckstation-web.worker.js" "${WEB_DIST}/"
  echo "✓ Copied duckstation-web.worker.js"
fi

# Copy data files if they exist
if [ -f "${BUILD_DIR}/bin/duckstation-web.data" ]; then
  cp "${BUILD_DIR}/bin/duckstation-web.data" "${WEB_DIST}/"
  DATA_SIZE=$(du -h "${BUILD_DIR}/bin/duckstation-web.data" | cut -f1)
  echo "✓ Copied duckstation-web.data (${DATA_SIZE})"
fi

echo ""
echo "=========================================="
echo " Build Complete!"
echo "=========================================="
echo ""
echo "Artifacts location: ${WEB_DIST}"
echo ""
echo "Files generated:"
ls -lh "${WEB_DIST}/" | tail -n +2 | awk '{print "  " $9 " (" $5 ")"}'
echo ""
echo "Next steps:"
echo "  1. Start the development server:"
echo "       bash scripts/serve-web.sh"
echo "  2. Open http://localhost:8080 in your browser"
echo ""
echo "=========================================="
