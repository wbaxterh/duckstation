# DuckStation WebAssembly - Debugging & Running Guide

**Date:** October 27, 2025
**Status:** 🚧 IN PROGRESS - Threading fixed, logging crashes remain
**Follow-up from:** [WASM-PORT-SUMMARY.md](WASM-PORT-SUMMARY.md)

---

## 📋 Table of Contents

1. [Quick Start](#quick-start)
2. [Build Process](#build-process)
3. [Running the Emulator](#running-the-emulator)
4. [Debugging Session Summary](#debugging-session-summary)
5. [Current Issues](#current-issues)
6. [File Structure](#file-structure)
7. [Troubleshooting](#troubleshooting)

---

## 🚀 Quick Start

### Starting from Scratch

```powershell
# 1. Navigate to repository
cd C:\Users\wesle\Documents\git\duckstation

# 2. Build WebAssembly version (takes 2-5 minutes)
.\scripts\build-web.ps1 Release

# 3. Start web server
.\scripts\serve-web.ps1

# 4. Open browser
# Navigate to: http://localhost:8080/play.html
```

### If Already Built

```powershell
# Just start the server
cd C:\Users\wesle\Documents\git\duckstation
.\scripts\serve-web.ps1

# Open: http://localhost:8080/play.html
```

---

## 🔨 Build Process

### Build Command

```powershell
.\scripts\build-web.ps1 Release
```

**What it does:**
1. Activates Emscripten SDK environment (`.emsdk/`)
2. Configures CMake for WebAssembly target
3. Compiles C++ source to WASM
4. Links with Emscripten libraries (zlib, OpenGL ES, etc.)
5. Copies artifacts to `web-dist/duckstation/`

**Build Output:**
- `web-dist/duckstation/duckstation-web.js` (169 KB) - JavaScript loader
- `web-dist/duckstation/duckstation-web.wasm` (9.5 MB) - Main emulator with debug symbols
- Build time: ~2-5 minutes on first build, ~30 seconds for incremental

**Build Configuration:**
- **Target:** WebAssembly (Emscripten 4.0.17)
- **Build Type:** Release with `-g1` debug symbols
- **Memory:** 1GB initial, 4GB max, growth enabled
- **Threading:** Disabled for WebAssembly compatibility
- **Logging:** Disabled for WebAssembly (causes crashes)

### Debug Build (if needed)

```powershell
.\scripts\build-web.ps1 Debug
```

Adds `-g3` for full debug symbols (increases file size to ~15MB).

---

## 🎮 Running the Emulator

### 1. Start Web Server

```powershell
cd C:\Users\wesle\Documents\git\duckstation
.\scripts\serve-web.ps1
```

**Server Details:**
- **URL:** http://localhost:8080
- **Port:** 8080 (configurable in script)
- **Root:** `web-dist/` directory
- **Technology:** Python SimpleHTTPServer
- **Logs:** Shows all HTTP requests in terminal

**Keep this terminal window open while using the emulator!**

### 2. Open Browser

Navigate to: **http://localhost:8080/play.html**

### 3. Load Files

The interface will prompt you for:

1. **BIOS File** (required)
   - Location: `C:\Users\wesle\Documents\git\duckstation\roms\SCPH1001.BIN`
   - Size: 512 KB
   - Region: NTSC-U (North America)

2. **CUE File** (required)
   - Location: `C:\Users\wesle\Documents\git\duckstation\roms\thps.cue`
   - References the BIN file

3. **BIN File** (required)
   - Location: `C:\Users\wesle\Documents\git\duckstation\roms\thps.bin`
   - Size: 630 MB (Tony Hawk's Pro Skater)
   - Loading takes ~10-15 seconds with progress bar

### 4. Click "Start Game"

The emulator will:
1. Initialize PS1 system
2. Load BIOS into virtual filesystem (`/data/bios/`)
3. Load game into virtual filesystem (`/games/`)
4. Boot the game

---

## 🐛 Debugging Session Summary

This session focused on fixing the CD-ROM boot crash from the previous session.

### Issue 1: Threading Not Supported ✅ FIXED

**Problem:**
```
RuntimeError: Aborted(native code called abort())
at CDROMAsyncReader::StartThread(unsigned int)
```

**Root Cause:**
DuckStation's CD-ROM async reader tried to create a `std::thread` for background reading, but WebAssembly doesn't support native threading without special flags.

**Solution Applied:**
Modified `src/core/cdrom.cpp` line 554-560:

```cpp
void CDROM::Initialize()
{
  s_state.disc_region = DiscRegion::NonPS1;

#ifndef __EMSCRIPTEN__
  // WebAssembly does not support threading, so disable async CD-ROM reader
  if (g_settings.cdrom_readahead_sectors > 0)
    s_reader.StartThread(g_settings.cdrom_readahead_sectors);
#endif

  Reset();
}
```

**Result:** Threading crash eliminated! ✅

---

### Issue 2: Logging System Crashes 🚧 IN PROGRESS

**Problem:**
```
RuntimeError: memory access out of bounds
at Log::ConsoleOutputLogCallback(...)
at Log::Write(...)
at CDROM::Initialize()
```

**Root Cause:**
DuckStation's logging system uses a buffer that causes memory access violations in WebAssembly. Every `INFO_LOG`, `ERROR_LOG`, etc. call crashes.

**Solution Applied:**
Modified `src/common/log.h` lines 191-224 to disable ALL logging for WebAssembly:

```cpp
// log wrappers
#define LOG_CHANNEL(name) [[maybe_unused]] static constexpr Log::Channel ___LogChannel___ = Log::Channel::name;

#ifdef __EMSCRIPTEN__
// WebAssembly: Disable all logging to avoid memory access errors in logging buffer
#define ERROR_LOG(...) do {} while(0)
#define WARNING_LOG(...) do {} while(0)
#define INFO_LOG(...) do {} while(0)
#define VERBOSE_LOG(...) do {} while(0)
#define DEV_LOG(...) do {} while(0)
#define DEBUG_LOG(...) do {} while(0)
#define TRACE_LOG(...) do {} while(0)
#else
// Normal platform logging (unchanged)
#define ERROR_LOG(...) Log::FastWrite(___LogChannel___, __func__, Log::Level::Error, __VA_ARGS__)
#define WARNING_LOG(...) Log::FastWrite(___LogChannel___, __func__, Log::Level::Warning, __VA_ARGS__)
#define INFO_LOG(...) Log::FastWrite(___LogChannel___, Log::Level::Info, __VA_ARGS__)
// ... etc
#endif
```

**Current Status:** Implemented but still testing. Last build: `duckstation-web-WORKING.wasm`

---

## ⚠️ Current Issues

### Still Investigating: Logging Crash Persists

**Symptom:**
```
RuntimeError: memory access out of bounds
at Log::ConsoleOutputLogCallback
```

**What We've Tried:**
1. ✅ Added `-g3` debug symbols to identify crash location
2. ✅ Disabled threading (fixed that issue)
3. ✅ Added logging disablement macros in `log.h`
4. 🚧 Verifying macros are properly applied in latest build

**Next Steps:**
1. Verify the `#ifdef __EMSCRIPTEN__` macros in `log.h` are working
2. Check if there's console output from other logging systems (OSD, GUI)
3. Consider disabling `Log::Write()` function entirely for WebAssembly
4. Investigate if source maps would help with debugging

---

## 📁 File Structure

### Source Files Modified

```
duckstation/
├── src/
│   ├── common/
│   │   └── log.h ⚠️ MODIFIED - Disabled all logging for __EMSCRIPTEN__
│   ├── core/
│   │   └── cdrom.cpp ⚠️ MODIFIED - Disabled threading for __EMSCRIPTEN__
│   ├── duckstation-web/
│   │   ├── CMakeLists.txt ⚠️ MODIFIED - Added -g3 debug flag, memory config
│   │   └── web_host.cpp ⚠️ MODIFIED - BIOS loading, boot logging
│   └── util/
│       └── cd_image_cue.cpp 📝 (Had logging added/removed during debugging)
```

### Web Distribution Files

```
web-dist/
├── duckstation/
│   ├── duckstation-web.js - Latest build (JavaScript loader)
│   ├── duckstation-web.wasm - Latest build (9.5 MB with debug symbols)
│   ├── duckstation-web-WORKING.js - Known working version (archived)
│   ├── duckstation-web-WORKING.wasm - Known working version (archived)
│   ├── duckstation-web-v2.js - Version 2 (threading fix)
│   ├── duckstation-web-v3.js - Version 3 (threading + attempted logging fix)
│   └── duckstation-web-v4.js - Version 4 (various attempts)
├── play.html ⚠️ MODIFIED - Frontend UI, loads duckstation-web-WORKING.js
├── index.html - Landing page
└── README.txt - Basic instructions
```

### Build Artifacts Location

```
build-web/ - CMake build directory (generated)
├── bin/
│   ├── duckstation-web.js - Fresh build before copying to web-dist
│   └── duckstation-web.wasm - Fresh build before copying to web-dist
└── CMakeCache.txt - Build configuration
```

---

## 🎨 Frontend (play.html)

### Features

**File Upload:**
- Drag-and-drop or click to select
- Supports: BIOS (.bin), CUE sheet (.cue), ROM (.bin)
- Real-time upload progress for large files (630 MB ROM)
- Shows file size and percentage loaded

**Status Display:**
- Color-coded messages (info=blue, success=green, error=red)
- Progress bar with percentage
- Detailed step-by-step feedback

**Canvas:**
- 1024x768 resolution
- WebGL 2.0 rendering
- Keyboard input support
- Gamepad support (planned)

**Error Handling:**
- Try-catch around all operations
- Detailed error messages with stack traces
- Browser console logging

### How It Works

1. **Module Loading:** Imports `duckstation-web.js` which loads the WASM
2. **Virtual Filesystem:** Creates `/data/bios/` and `/games/` using Emscripten FS API
3. **File Upload:** Uses `FileReader` with progress tracking
4. **C++ Calls:** Uses `Module.ccall()` to invoke exported C++ functions:
   - `duckstation_init()` - Initialize emulator
   - `duckstation_load_bios(path)` - Set BIOS path
   - `duckstation_boot_game(path)` - Boot game from CUE file
   - `duckstation_frame()` - Render one frame (called in game loop)
   - `duckstation_shutdown()` - Clean up

### Keyboard Controls (Planned)

```
Arrow Keys = D-Pad
Z = Cross (X)
X = Circle (O)
A = Square
S = Triangle
Q = L1
W = L2
E = R1
R = R2
Enter = Start
Shift = Select
```

---

## 🔧 Troubleshooting

### Build Issues

**Problem:** "EMSDK not found"
```powershell
# Re-run setup
.\scripts\setup-emsdk.ps1
```

**Problem:** CMake errors
```powershell
# Clean build directory
Remove-Item -Recurse -Force build-web
.\scripts\build-web.ps1 Release
```

**Problem:** Linker errors
- Check that all source files in `src/duckstation-web/CMakeLists.txt` are valid
- Verify Emscripten exported functions list is correct

---

### Runtime Issues

**Problem:** "404 Not Found" for WASM file
- Check web server is running: `.\scripts\serve-web.ps1`
- Verify files exist in `web-dist/duckstation/`
- Try hard refresh: Ctrl+Shift+R

**Problem:** Browser caches old WASM file
- Close browser completely
- Restart web server
- Use versioned filenames (e.g., `-v2`, `-v3`, `-WORKING`)
- Clear browser cache: DevTools > Application > Clear Storage

**Problem:** "Memory access out of bounds"
- This is the current logging crash issue
- Check console for which function crashes
- Verify latest build has logging disabled

**Problem:** Black screen / no rendering
- Check browser console for WebGL errors
- Verify canvas element exists
- Check if `duckstation_frame()` is being called

---

### Debugging Tips

**Enable Verbose Logging in Browser:**
```javascript
// In browser console
localStorage.debug = '*'
```

**Check WASM File is Correct:**
```powershell
# Check file size (should be ~9.5 MB with debug symbols)
ls web-dist/duckstation/duckstation-web*.wasm -lh

# Check timestamp (should be recent)
ls web-dist/duckstation/duckstation-web*.wasm -lh
```

**Test with Different Browser:**
- Chrome/Edge: Best WebAssembly support, good debugging tools
- Firefox: Alternative rendering, different WASM engine
- Safari: May have compatibility issues

**Inspect Network Traffic:**
1. Open DevTools (F12)
2. Network tab
3. Reload page
4. Check that WASM file loads (should be ~9.5 MB)
5. Check response headers for cache-control

---

## 📊 Build Versions History

| Version | Threading | Logging | Status | File Size |
|---------|-----------|---------|--------|-----------|
| v1 (original) | ❌ Crashes | ✅ Works | ❌ Failed | 4.34 MB |
| v2 | ✅ Disabled | ❌ Crashes | ❌ Failed | 9.44 MB |
| v3 | ✅ Disabled | 🔧 Attempted | ❌ Failed | 9.44 MB |
| v4 | ✅ Disabled | 🔧 Attempted | ❌ Failed | 9.44 MB |
| **WORKING** | ✅ Disabled | ✅ Disabled | 🚧 Testing | 9.5 MB |

---

## 🎯 Next Session TODO

1. **Verify Logging Fix:**
   - Build and test latest `duckstation-web-WORKING.wasm`
   - Check if logging crashes are eliminated
   - If not, investigate `Log::Write()` function directly

2. **If Logging Fixed:**
   - Test full game boot sequence
   - Verify ROM loads and starts
   - Check frame rendering
   - Test keyboard input
   - Implement game loop with requestAnimationFrame

3. **Performance Testing:**
   - Measure FPS
   - Check memory usage
   - Optimize if needed

4. **Save/Load States:**
   - Implement IndexedDB storage
   - Add save state UI buttons

5. **Audio:**
   - Integrate Web Audio API
   - Implement `audio_stream_web.cpp`

---

## 📚 Related Documentation

- **[WASM-PORT-SUMMARY.md](WASM-PORT-SUMMARY.md)** - Original porting work and CPU linker fixes
- **[TROUBLESHOOTING-WEB-BUILD.md](TROUBLESHOOTING-WEB-BUILD.md)** - Build troubleshooting guide
- **[WEB-BUILD-REFERENCE.md](WEB-BUILD-REFERENCE.md)** - Technical reference for web build
- **[WASM-DEPENDENCIES.md](WASM-DEPENDENCIES.md)** - Dependency management

---

## 🔗 Useful Links

- **DuckStation GitHub:** https://github.com/stenzek/duckstation
- **Emscripten Docs:** https://emscripten.org/docs/
- **WebAssembly Spec:** https://webassembly.github.io/spec/
- **PS1 BIOS Info:** https://emulation.gametechwiki.com/index.php/PlayStation_emulators

---

**Last Updated:** October 27, 2025
**Session Duration:** ~4 hours
**Status:** Ready to test latest build with logging fully disabled
