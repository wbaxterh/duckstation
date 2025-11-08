# DuckStation Web Build - Quick Reference

**Complete web frontend for DuckStation PlayStation 1 emulator using WebAssembly/Emscripten**

---

## 🚀 Quick Start

### Windows (PowerShell) - EASIEST METHOD
```powershell
# Option A: One-command build (recommended for first time)
.\scripts\build-web-complete.ps1

# Then start server:
.\scripts\serve-web.ps1

# Option B: Manual steps (if you need more control)
.\scripts\setup-emsdk.ps1    # 1. Install SDK
.\scripts\build-web.ps1       # 2. Build (auto-activates emsdk)
.\scripts\serve-web.ps1       # 3. Start server
```

### Linux/macOS (bash)
```bash
# 1. Install Emscripten SDK
bash scripts/setup-emsdk.sh

# 2. Activate environment & build
source .emsdk/emsdk_env.sh
bash scripts/build-web.sh

# 3. Start dev server
bash scripts/serve-web.sh
```

Open http://localhost:8080 → Upload BIOS + ROM → Boot!

---

## 📁 What Was Created

### Build Scripts
- **`scripts/setup-emsdk.sh`** - Installs Emscripten SDK to `.emsdk/`
- **`scripts/build-web.sh`** - Compiles DuckStation to WASM
- **`scripts/serve-web.sh`** - Dev server with COOP/COEP headers

### Web Frontend
- **`src/duckstation-web/web_host.cpp`** - Minimal web-based host
- **`src/duckstation-web/CMakeLists.txt`** - Emscripten build config

### Web UI
- **`web-dist/index.html`** - Browser UI with file upload
- **`web-dist/style.css`** - Responsive dark theme
- **`web-dist/duckstation/`** - Build output directory

### Documentation
- **`docs/BUILD-WEB.md`** - Complete guide (troubleshooting, architecture, etc.)
- **`web-dist/duckstation/README.txt`** - Output files reference

### Git
- **`.gitignore`** - Excludes build artifacts, ROMs, BIOS files

---

## 🏗️ Architecture Overview

### CPU Execution
- **Native builds:** JIT Recompiler (fast)
- **Web build:** Cached Interpreter (WASM-compatible, ~50-70% speed)
- Forced in `web_host.cpp:72`: `ExecutionMode = "CachedInterpreter"`

### Rendering
- WebGL 2.0 via OpenGL ES emulation
- Software renderer fallback
- Canvas-based output

### File System
- Emscripten Virtual FS (MEMFS)
- JavaScript uploads → `Module.FS.writeFile()`
- Paths: `/bios/`, `/roms/`, `/data/`

### Threading
- **Current:** Single-threaded
- **Future:** Add `-pthread` flag for multi-threading
- COOP/COEP headers already configured in dev server

---

## 🔧 Key Files & Functions

### Exported C Functions (callable from JavaScript)
```cpp
duckstation_init()          // Initialize emulator
duckstation_load_bios(path) // Load BIOS file
duckstation_boot_game(path) // Boot game
duckstation_frame()         // Execute one frame
duckstation_shutdown()      // Clean shutdown
```

### JavaScript Integration (index.html)
```javascript
import createDuckStationModule from './duckstation/duckstation-web.js';

const Module = await createDuckStationModule({
  canvas: document.getElementById('screen'),
  locateFile: (path) => `/duckstation/${path}`
});

// Mount files
Module.FS.writeFile('/bios/scph1001.bin', biosData);
Module.FS.writeFile('/roms/game.cue', gameData);

// Initialize & boot
Module.ccall('duckstation_init', 'number', [], []);
Module.ccall('duckstation_boot_game', 'number', ['string'], ['/roms/game.cue']);

// Render loop
function frame() {
  Module.ccall('duckstation_frame', null, [], []);
  requestAnimationFrame(frame);
}
frame();
```

---

## ⚙️ Build Configuration

### CMake Options (build-web.sh)
```bash
emcmake cmake -B build-web \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DENABLE_OPENGL=ON \
  -DENABLE_VULKAN=OFF \
  -DBUILD_QT_FRONTEND=OFF \
  -DBUILD_MINI_FRONTEND=OFF
```

### Emscripten Flags (src/duckstation-web/CMakeLists.txt)
```cmake
-s MODULARIZE=1                    # ES6 module
-s EXPORT_ES6=1                    # ES6 syntax
-s USE_WEBGL2=1                    # WebGL 2.0
-s ALLOW_MEMORY_GROWTH=1           # Dynamic memory
-s FORCE_FILESYSTEM=1              # Virtual FS
-s INITIAL_MEMORY=256MB            # Starting heap
-s MAXIMUM_MEMORY=2GB              # Max heap
-s EXPORTED_FUNCTIONS=[...]        # C functions for JS
```

---

## 🐛 Common Issues & Fixes

### Issue: `emcc: command not found` (Windows)

**The fix is already built into the script!** As of the latest update, `build-web.ps1` automatically activates the emsdk environment, so you don't need to manually activate it anymore.

Just run:
```powershell
.\scripts\build-web.ps1
```

If you still get errors, try the complete build:
```powershell
.\scripts\build-web-complete.ps1
```

### Issue: `emcc: command not found` (Linux/macOS)

**Manual activation still needed on Linux/macOS:**
```bash
source .emsdk/emsdk_env.sh
bash scripts/build-web.sh
```

### Issue: SharedArrayBuffer not available
**Dev server already has COOP/COEP headers!**

For production (Nginx):
```nginx
add_header Cross-Origin-Opener-Policy same-origin;
add_header Cross-Origin-Embedder-Policy require-corp;
```

### Issue: Low performance
1. Use Release build: `bash scripts/build-web.sh Release`
2. Cached Interpreter is ~2x slower than JIT (WASM limitation)
3. Consider lighter games or reduce resolution

### Issue: Game won't boot
1. Check browser console (F12) for errors
2. Verify BIOS format (.bin, not .exe)
3. Ensure game format is supported (.cue/.bin/.iso/.chd)

---

## 📊 Performance Expectations

| Component | Native (JIT) | Web (Interpreter) |
|-----------|--------------|-------------------|
| CPU Speed | 100% | 50-70% |
| GPU Rendering | Hardware | WebGL (variable) |
| Audio | Native APIs | Web Audio API |
| Threading | Multi-thread | Single (upgradable) |

**Playable games:** Most 2D games, simpler 3D titles
**Challenging:** Complex 3D games (Metal Gear Solid, FF7)

---

## 🚢 Deployment

### Static Hosting (GitHub Pages, Netlify, Vercel)
1. Copy `web-dist/` to hosting
2. Set headers (Netlify `_headers` example):
```
/*
  Cross-Origin-Opener-Policy: same-origin
  Cross-Origin-Embedder-Policy: require-corp
```

### Self-Hosted (Apache/Nginx)
```apache
# Apache (.htaccess)
Header set Cross-Origin-Opener-Policy "same-origin"
Header set Cross-Origin-Embedder-Policy "require-corp"
```

---

## 🔬 Advanced: Adding Pthread Support

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

Requires COOP/COEP (already in serve script).

---

## 📚 Further Reading

- **Full Documentation:** `docs/BUILD-WEB.md`
- **Emscripten Docs:** https://emscripten.org/docs/
- **DuckStation GitHub:** https://github.com/stenzek/duckstation

---

## 🎯 Summary

You now have:
✅ Complete WASM build system for DuckStation
✅ Self-hosted browser UI with file upload
✅ Dev server with proper security headers
✅ Cached Interpreter CPU (no JIT, WASM-compatible)
✅ WebGL rendering pipeline
✅ Comprehensive documentation

**Next Steps:**
1. Build: `bash scripts/build-web.sh`
2. Test: Upload BIOS + ROM at http://localhost:8080
3. Optimize: Tune Emscripten flags for your use case
4. Deploy: Host on your server with COOP/COEP headers

**Happy Emulating! 🦆🎮**
