#!/usr/bin/env bash
# ==============================================================================
# DuckStation Web Build - Emscripten SDK Setup Script
# ==============================================================================
# This script downloads and configures the Emscripten SDK locally within the
# repository. No global installation required.
#
# Usage: bash scripts/setup-emsdk.sh
#
# The SDK will be installed to: <REPO>/.emsdk/
# ==============================================================================

set -euo pipefail

# Determine the repository root (script is in <REPO>/scripts/)
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
EMSDK_DIR="${REPO_ROOT}/.emsdk"

echo "=========================================="
echo " DuckStation - Emscripten SDK Setup"
echo "=========================================="
echo ""
echo "Repository root: ${REPO_ROOT}"
echo "Installing emsdk to: ${EMSDK_DIR}"
echo ""

# Clone emsdk if it doesn't exist
if [ ! -d "${EMSDK_DIR}" ]; then
  echo "[1/4] Cloning Emscripten SDK..."
  git clone https://github.com/emscripten-core/emsdk.git "${EMSDK_DIR}"
else
  echo "[1/4] Emscripten SDK already cloned. Updating..."
  cd "${EMSDK_DIR}"
  git pull
  cd "${REPO_ROOT}"
fi

# Install latest SDK
echo ""
echo "[2/4] Installing latest Emscripten SDK..."
cd "${EMSDK_DIR}"
./emsdk install latest

# Activate latest SDK
echo ""
echo "[3/4] Activating latest Emscripten SDK..."
./emsdk activate latest

# Generate activation script path
EMSDK_ENV="${EMSDK_DIR}/emsdk_env.sh"

echo ""
echo "[4/4] Setup complete!"
echo ""
echo "=========================================="
echo " IMPORTANT: Next Steps"
echo "=========================================="
echo ""
echo "Before building, you MUST source the emsdk environment:"
echo ""
echo "  source ${EMSDK_ENV}"
echo ""
echo "Or add this to your shell profile (~/.bashrc, ~/.zshrc):"
echo ""
echo "  export EMSDK='${EMSDK_DIR}'"
echo "  source \"\${EMSDK}/emsdk_env.sh\" > /dev/null 2>&1"
echo ""
echo "Then run the build script:"
echo ""
echo "  bash scripts/build-web.sh"
echo ""
echo "=========================================="
