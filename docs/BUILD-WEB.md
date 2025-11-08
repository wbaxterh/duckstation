# DuckStation Web Build Guide

Complete guide to building and running DuckStation in your browser via WebAssembly.

## Table of Contents

1. [Overview](#overview)
2. [Prerequisites](#prerequisites)
3. [Quick Start](#quick-start)
4. [Build Process Explained](#build-process-explained)
5. [Architecture & Design](#architecture--design)
6. [Troubleshooting](#troubleshooting)
7. [Advanced Topics](#advanced-topics)

---

## Overview

This web build allows DuckStation to run entirely in your browser using WebAssembly (WASM). It's self-hosted, meaning you can deploy it on your own server without external dependencies.

### Key Features
- ✅ Full PS1 emulation in browser
- ✅ Cached Interpreter CPU (WASM-compatible, no JIT)
- ✅ WebGL rendering
- ✅ File upload for BIOS/ROMs (no server storage)
- ✅ Keyboard and gamepad support
- ✅ COOP/COEP headers for SharedArrayBuffer (future threading)
- ✅ Self-contained build with no CDN dependencies

### Limitations
- ❌ No JIT recompiler (uses slower Cached Interpreter)
- ❌ Single-threaded initially (can be upgraded to pthreads)
- ❌ Performance ~50-70% of native (varies by game)
- ❌ Limited audio backends (Web Audio API)

---

## Prerequisites

### System Requirements
- **OS:** macOS, Linux, or WSL2 on Windows
- **Tools:**
  - Git
  - CMake 3.19+
  - Ninja build system
  - Python 3 or Node.js (for dev server)
  - 4GB+ RAM for building
  - 10GB+ disk space

### Installation

**macOS (Homebrew):**
```bash
brew install cmake ninja python3
```

**Ubuntu/Debian:**
```bash
sudo apt update
sudo apt install -y cmake ninja-build python3 git
```

**Fedora:**
```bash
sudo dnf install cmake ninja-build python3 git
```

---

## Quick Start

### Step 1: Clone Repository
```bash
cd ~/src  # or your preferred location
git clone https://github.com/stenzek/duckstation.git
cd duckstation
```

### Step 2: Install Emscripten SDK
```bash
bash scripts/setup-emsdk.sh
```

This installs Emscripten locally to `.emsdk/` (no global install needed).

### Step 3: Activate Emscripten Environment
```bash
source .emsdk/emsdk_env.sh
```

**Tip:** Add to your shell profile (~/.bashrc or ~/.zshrc):
```bash
export EMSDK="$HOME/src/duckstation/.emsdk"
source "$EMSDK/emsdk_env.sh" > /dev/null 2>&1
```

### Step 4: Build
```bash
bash scripts/build-web.sh
```

Build types:
- `bash scripts/build-web.sh RelWithDebInfo` (default, recommended)
- `bash scripts/build-web.sh Release` (optimized, smaller binary)
- `bash scripts/build-web.sh Debug` (debugging symbols)

### Step 5: Run Development Server
```bash
bash scripts/serve-web.sh
```

### Step 6: Open Browser
Navigate to: **http://localhost:8080**

1. Upload a PS1 BIOS file (e.g., `scph1001.bin`)
2. Upload a game image (.cue, .bin, .iso, or .chd)
3. Click **🚀 Boot Game**

---

## Build Process Explained

### What Happens During Build

#### 1. **CMake Configuration** (`emcmake cmake ...`)
- Sets `EMSCRIPTEN=1` to activate web frontend
- Disables Qt, Vulkan, and native-only features
- Forces Cached Interpreter CPU mode (no JIT in WASM)
- Configures OpenGL ES renderer for WebGL compatibility

#### 2. **Compilation** (`cmake --build`)
- Compiles C++ source to LLVM bytecode
- Links with DuckStation core libraries (core, util, common)
- Applies Emscripten-specific optimizations

#### 3. **Linking** (Emscripten linker)
Generates:
- `duckstation-web.js` - JavaScript module
- `duckstation-web.wasm` - WebAssembly binary
- `duckstation-web.worker.js` - Web Worker (if pthreads enabled)

Key linker flags (see `src/duckstation-web/CMakeLists.txt`):
```cmake
-s MODULARIZE=1           # ES6 module export
-s EXPORT_ES6=1           # Use ES6 syntax
-s USE_WEBGL2=1           # WebGL 2.0 support
-s ALLOW_MEMORY_GROWTH=1  # Dynamic memory
-s FORCE_FILESYSTEM=1     # Virtual FS for ROMs
```

#### 4. **Installation**
Copies artifacts to `web-dist/duckstation/`:
- `duckstation-web.js`
- `duckstation-web.wasm`
- Optional worker/data files

### Output Structure
```
duckstation/
├── web-dist/
│   ├── index.html          # Main UI
│   ├── style.css           # Stylesheet
│   └── duckstation/
│       ├── duckstation-web.js
│       ├── duckstation-web.wasm
│       └── README.txt
├── scripts/
│   ├── setup-emsdk.sh
│   ├── build-web.sh
│   └── serve-web.sh
├── src/duckstation-web/
│   ├── web_host.cpp        # Web frontend
│   └── CMakeLists.txt
└── .emsdk/                 # Emscripten SDK
```

---

## Architecture & Design

### CPU Execution Mode

**Native builds** use JIT recompiler for speed:
- Recompiler (x86-64, ARM64, RISC-V) → JIT compilation

**Web build** uses interpreter:
- Cached Interpreter → Pre-decoded instruction cache
- ~50-70% performance vs native JIT
- WASM-compatible (no dynamic code generation)

Configured in `src/duckstation-web/web_host.cpp:72`:
```cpp
si.SetStringValue("CPU", "ExecutionMode", "CachedInterpreter");
```

### Rendering

**WebGL 2.0** via Emscripten's OpenGL ES emulation:
- Software renderer as fallback
- Hardware acceleration where available
- Shaders compiled to WebGL GLSL

### File System

**Emscripten Virtual FS (MEMFS)**:
- JavaScript uploads files → `Module.FS.writeFile()`
- Mounted paths:
  - `/bios/` - BIOS files
  - `/roms/` - Game images
  - `/data/` - Save states, settings

No server-side storage; everything is in-browser.

### Threading (Future)

Current: Single-threaded (main thread handles CPU + GPU + Audio)

Future: Multi-threaded with pthreads:
- Requires `SharedArrayBuffer`
- Needs COOP/COEP headers (already configured in serve script)
- Add `-pthread` to Emscripten flags

---

## Troubleshooting

### Build Issues

#### Error: `emcc: command not found`
**Solution:**
```bash
source .emsdk/emsdk_env.sh
```

#### Error: CMake configuration failed
**Possible causes:**
1. Missing dependencies in `dep/` folder
2. Incompatible CMake options

**Debug steps:**
```bash
# Check CMake version
cmake --version  # Need 3.19+

# Clean build directory
rm -rf build-web
bash scripts/build-web.sh
```

#### Error: Linker fails with "undefined symbol"
**Cause:** Missing exported function or library

**Check:**
1. Verify function is exported in `CMakeLists.txt`:
   ```cmake
   -s EXPORTED_FUNCTIONS=['_main','_duckstation_init',...]
   ```
2. Ensure all dependencies are linked:
   ```cmake
   target_link_libraries(duckstation-web PRIVATE core util common)
   ```

### Runtime Issues

#### SharedArrayBuffer not available
**Symptoms:** Browser console shows `SharedArrayBuffer is not defined`

**Solution:** Server must send COOP/COEP headers. Our dev server (`serve-web.sh`) handles this automatically.

For production (e.g., Nginx):
```nginx
add_header Cross-Origin-Opener-Policy same-origin;
add_header Cross-Origin-Embedder-Policy require-corp;
```

#### WebGL not available
**Check browser support:** https://get.webgl.org/

**Fallback:** Software renderer (slower):
```cpp
si.SetStringValue("GPU", "Renderer", "Software");
```

#### Audio doesn't play
**Cause:** Browser requires user gesture to start AudioContext.

**Solution:** Already handled in `index.html` - audio starts after "Boot Game" button click.

#### Game doesn't boot
**Debug steps:**
1. Open browser console (F12)
2. Check for errors in `[DuckStation]` logs
3. Verify BIOS file is correct format (.bin)
4. Ensure game image format is supported (.cue/.bin/.iso/.chd)

### Performance Issues

#### Low FPS (<30 FPS)
**Causes:**
1. Cached Interpreter is slower than JIT
2. Browser overhead
3. Large games/complex scenes

**Optimizations:**
1. Use Release build: `bash scripts/build-web.sh Release`
2. Reduce internal resolution (add to settings)
3. Disable enhancements (PGXP, texture filtering)

#### High Memory Usage
**Cause:** WASM memory growth

**Solution:** Set memory limits in CMakeLists.txt:
```cmake
-s INITIAL_MEMORY=256MB
-s MAXIMUM_MEMORY=2GB
```

---

## Advanced Topics

### Adding pthread Support

Edit `src/duckstation-web/CMakeLists.txt`:
```cmake
set(EMSCRIPTEN_LINK_FLAGS
  ...
  "-pthread"
  "-s PTHREAD_POOL_SIZE=4"
  "-s PROXY_TO_PTHREAD=1"
)
```

Rebuild:
```bash
bash scripts/build-web.sh
```

**Note:** Requires COOP/COEP headers (already in `serve-web.sh`).

### Packaging Resources

Pre-load data files (e.g., shaders, fonts):
```cmake
--preload-file ${CMAKE_SOURCE_DIR}/data/resources@/app/resources
```

Generates `duckstation-web.data` file loaded at startup.

### Custom CMake Options

Discover available options:
```bash
grep -r "option(" CMakeModules/
```

Example - disable specific features:
```bash
emcmake cmake -B build-web \
  -DENABLE_OPENGL=ON \
  -DENABLE_VULKAN=OFF \
  -DBUILD_QT_FRONTEND=OFF
```

### Deployment

**Static hosting** (GitHub Pages, Netlify, etc.):
1. Upload `web-dist/` contents
2. Configure server headers:
   ```
   Cross-Origin-Opener-Policy: same-origin
   Cross-Origin-Embedder-Policy: require-corp
   ```

**Netlify example** (_headers file):
```
/*
  Cross-Origin-Opener-Policy: same-origin
  Cross-Origin-Embedder-Policy: require-corp
  Cache-Control: no-cache
```

### Debug Builds

Enable assertions and debug symbols:
```bash
bash scripts/build-web.sh Debug
```

Add source maps in CMakeLists.txt:
```cmake
"-g4"  # Full debug info
"--source-map-base http://localhost:8080/"
```

View C++ source in browser DevTools!

---

## Fallback: RetroArch Web (If Needed)

If direct DuckStation web build proves too complex, use RetroArch:

1. Build DuckStation as Libretro core
2. Use RetroArch web player (pre-built Emscripten frontend)
3. Load DuckStation core + ROM via RetroArch UI

**Trade-off:** Less customization, but proven stable.

---

## References

- [Emscripten Documentation](https://emscripten.org/docs/)
- [WebAssembly](https://webassembly.org/)
- [DuckStation GitHub](https://github.com/stenzek/duckstation)
- [SharedArrayBuffer Security](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/SharedArrayBuffer#security_requirements)

---

## Contributing

Found a bug or have improvements?
1. Check existing issues: https://github.com/stenzek/duckstation/issues
2. Submit pull requests for web frontend: `src/duckstation-web/`
3. Update this guide: `docs/BUILD-WEB.md`

---

**Happy Emulating! 🦆🎮**
