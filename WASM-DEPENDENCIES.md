# DuckStation WebAssembly Build - Dependency Mapping

This document tracks all desktop dependencies that are skipped or replaced in the WASM build, ensuring we maintain full functionality for playing PS1 games in the browser.

**Status Legend:**
- ✅ **Replaced** - Native functionality replaced with web equivalent
- ⚠️ **Partial** - Some functionality available, limitations exist
- ❌ **Missing** - Not yet implemented, may need work
- 🔧 **Needs Testing** - Implementation exists but untested

---

## Core System Libraries

### Graphics & Rendering

| Desktop Library | Status | WASM Replacement | Notes |
|-----------------|--------|------------------|-------|
| **OpenGL (desktop)** | ✅ Replaced | **WebGL 2.0** via Emscripten | Emscripten automatically translates OpenGL ES calls to WebGL. Our OpenGL renderer works via `-s USE_WEBGL2=1` flag. |
| **Vulkan** | ❌ Skipped | N/A | Not available in browsers. OpenGL/WebGL path is required. Disabled via `-DENABLE_VULKAN=OFF`. |
| **Direct3D 11/12** | ❌ Skipped | N/A | Windows-only, not applicable to web builds. |
| **Metal** | ❌ Skipped | N/A | macOS-only, not applicable to web builds. |
| **GLAD (OpenGL loader)** | ✅ Replaced | Emscripten's GL emulation | Emscripten provides GL function pointers automatically. |
| **EGL/GLX/WGL** | ✅ Replaced | Browser WebGL context | Context creation handled by browser, not needed. |

**Verification Needed:**
- [ ] Test that OpenGL device creation works (`opengl_device.cpp`)
- [ ] Verify shader compilation for WebGL (GLSL ES compatibility)
- [ ] Test texture uploads and framebuffer operations

---

### Audio

| Desktop Library | Status | WASM Replacement | Notes |
|-----------------|--------|------------------|-------|
| **SDL3 Audio** | ✅ Replaced | **Web Audio API** | Emscripten provides SDL2 emulation via Web Audio API with `-s USE_SDL=2`. We'll need to use Emscripten's audio abstraction. |
| **Cubeb** | ✅ Replaced | Web Audio API | Not needed - Web Audio API handles all browser audio. |

**Implementation Status:**
- ⚠️ **Current:** We're skipping `cubeb_audio_stream.cpp` and `sdl_audio_stream.cpp`
- 🔧 **Needs:** Web-specific audio stream implementation (`web_audio_stream.cpp`)

**Action Items:**
- [ ] Create `src/util/web_audio_stream.cpp` that uses Emscripten's audio queue
- [ ] Ensure audio buffer size matches PS1 output (44.1kHz, 2-channel stereo)
- [ ] Test audio sync with frame rendering

---

### Input

| Desktop Library | Status | WASM Replacement | Notes |
|-----------------|--------|------------------|-------|
| **SDL3 Input** | ✅ Replaced | **Gamepad API + Keyboard Events** | Browser provides native gamepad support. Emscripten's SDL emulation handles this. |
| **DInput (Windows)** | ❌ Skipped | N/A | Not needed in browser. |
| **XInput (Windows)** | ❌ Skipped | N/A | Not needed in browser. |
| **Raw Input (Windows)** | ❌ Skipped | N/A | Not needed in browser. |

**Implementation Status:**
- ⚠️ **Current:** We're skipping `sdl_input_source.cpp`, `dinput_source.cpp`, `xinput_source.cpp`
- 🔧 **Needs:** Web input source implementation

**Action Items:**
- [ ] Verify Emscripten's SDL2 gamepad support works with PS1 controller mapping
- [ ] Test keyboard input for emulator controls
- [ ] Add on-screen touch controls for mobile (future enhancement)

---

### Window Management

| Desktop Library | Status | WASM Replacement | Notes |
|-----------------|--------|------------------|-------|
| **SDL3 Window** | ✅ Replaced | **HTML5 Canvas** | Emscripten creates a canvas element, no windowing needed. |
| **X11** | ❌ Skipped | N/A | Not applicable to web. |
| **Wayland** | ❌ Skipped | N/A | Not applicable to web. |

**Verification Needed:**
- [ ] Test canvas resizing
- [ ] Verify fullscreen mode works

---

## File I/O & Compression

### Image Formats

| Desktop Library | Status | WASM Replacement | Notes |
|-----------------|--------|------------------|-------|
| **libpng** | ⚠️ Skipped | **Emscripten port** or browser Image API | We may need to add `-s USE_LIBPNG=1` if we're loading PNG textures/icons. For now, skipped. |
| **libjpeg** | ⚠️ Skipped | **Emscripten port** or browser Image API | Skipped, but may be needed for screenshots or UI assets. |
| **libwebp** | ⚠️ Skipped | Native browser support | Browsers natively decode WebP. May not need the library unless we're encoding. |

**Current Issue:**
- ❌ We removed `JPEG::JPEG`, `PNG::PNG`, `WebP::libwebp` from util CMakeLists.txt
- 🔧 **Risk:** If DuckStation loads cover art, memory card icons, or texture replacements in these formats, it will fail

**Action Items:**
- [ ] Audit code for PNG/JPEG/WebP usage (likely in `image.cpp`, `animated_image.cpp`)
- [ ] Add Emscripten ports if needed: `-s USE_LIBPNG=1 -s USE_LIBJPEG=1`
- [ ] Alternative: Use browser's `Image()` API for loading textures via JS

---

### Compression

| Desktop Library | Status | WASM Replacement | Notes |
|-----------------|--------|------------------|-------|
| **ZLIB** | ✅ Replaced | **Emscripten port** (`-s USE_ZLIB=1`) | Critical for CHD/ZIP support. We created a dummy target but should use Emscripten's port. |
| **zstd** | ⚠️ Skipped | **Emscripten port** or bundled | Skipped, but may be needed for compressed save states. |
| **lzma** | ✅ Kept | Bundled in `dep/` | Likely used for 7z/CHD, should work as-is. |
| **libchdr** | ✅ Kept | Bundled in `dep/` | Essential for CHD disc images, kept in build. |

**Current Issue:**
- ⚠️ We removed `zstd::libzstd_shared` but kept `zstd` search in dependencies
- 🔧 **Risk:** Compressed save states or CHD with zstd compression may fail

**Action Items:**
- [ ] Add `-s USE_ZLIB=1` to Emscripten linker flags
- [ ] Verify CHD disc images load correctly
- [ ] Test loading .zip files with ROMs (if supported)
- [ ] Re-add zstd if save states use it

---

### Disc Image Formats

| Component | Status | Notes |
|-----------|--------|-------|
| **CUE/BIN** | ✅ Should work | Text parsing, no external deps. In `cd_image_cue.cpp`. |
| **ISO** | ✅ Should work | Raw file reading, no external deps. |
| **CHD** | ✅ Should work | Uses libchdr (kept in build). |
| **PBP (PSP)** | ✅ Should work | Self-contained parsing. |
| **M3U** | ✅ Should work | Text parsing for multi-disc. |
| **MDS/MDF** | ✅ Should work | Binary parsing. |
| **PPF Patches** | ✅ Should work | In `cd_image_ppf.cpp`. |

**Verification Needed:**
- [ ] Test loading .cue/.bin via JavaScript File API
- [ ] Test loading .chd files
- [ ] Test multi-disc M3U switching

---

## Networking & Online Features

| Desktop Library | Status | WASM Replacement | Notes |
|-----------------|--------|------------------|-------|
| **libcurl** | ✅ Replaced | **Fetch API** (`emscripten_fetch`) | Emscripten provides fetch API for HTTP requests. |
| **WinHTTP** | ❌ Skipped | Fetch API | Windows-only, not needed. |

**Implementation Status:**
- ⚠️ **Current:** We skipped `http_downloader_curl.cpp` and `http_downloader_winhttp.cpp`
- 🔧 **Needs:** Web-specific HTTP downloader (`http_downloader_web.cpp`)

**Use Cases:**
- RetroAchievements API
- Downloading game covers/metadata
- Updating cheats/database

**Action Items:**
- [ ] Implement `http_downloader_web.cpp` using `emscripten_fetch_*` APIs
- [ ] Test RetroAchievements login and achievement unlocking
- [ ] Verify CORS headers allow external requests

---

## System Services

### D-Bus (Linux)

| Component | Status | Notes |
|-----------|--------|-------|
| **D-Bus** | ❌ Skipped | Not applicable to web. Used for system notifications on Linux. |

**Impact:** No system notifications, but not critical for gameplay.

---

### Device Detection

| Desktop Library | Status | WASM Replacement | Notes |
|-----------------|--------|------------------|-------|
| **udev (Linux)** | ❌ Skipped | Browser Gamepad API | Used to detect controllers on Linux. |

**Impact:** Browsers handle device detection automatically. No loss of functionality.

---

## Shader Compilation

| Desktop Library | Status | WASM Replacement | Notes |
|-----------------|--------|------------------|-------|
| **Shaderc** | ⚠️ Skipped | **Runtime GLSL compilation** | We removed Shaderc include dirs. Risk if we compile SPIR-V shaders at runtime. |
| **SPIRV-Cross** | ⚠️ Skipped | N/A | Used to translate SPIR-V to GLSL. May not be needed if we use GLSL directly. |

**Current Issue:**
- ❌ Removed Shaderc/SPIRV-Cross includes
- 🔧 **Risk:** Post-processing shaders, enhancements (PGXP textures), and ReShade effects may fail

**How DuckStation Uses Shaders:**
- **Core GPU:** Software renderer or OpenGL with GLSL shaders (no SPIR-V)
- **Post-processing:** ReShade FX format, compiled to GLSL via `reshadefx` library
- **Enhancements:** Runtime GLSL generation for texture filters, upscaling

**Action Items:**
- [ ] Verify that `reshadefx` library works in WASM (we kept it in the build)
- [ ] Test basic rendering without post-processing first
- [ ] Add post-processing shaders and test
- [ ] Check if any shaders use SPIR-V path (unlikely for OpenGL)

---

## Font Rendering & UI

| Desktop Library | Status | WASM Replacement | Notes |
|-----------------|--------|------------------|-------|
| **FreeType** | ⚠️ Skipped | **Emscripten port** (`-s USE_FREETYPE=1`) | Used by ImGui for font rasterization. May need to add. |
| **Harfbuzz** | ⚠️ Skipped | May not be needed | Used for advanced text shaping (Arabic, etc.). ImGui may not require it. |
| **PlutoSVG** | ⚠️ Skipped | May not be needed | For SVG icons. If UI uses SVG assets, we'll need this. |

**Current Issue:**
- ❌ Removed FreeType, Harfbuzz, PlutoSVG from util linking
- 🔧 **Risk:** ImGui UI may not render text properly

**Action Items:**
- [ ] Test if ImGui works without FreeType (it has a default font)
- [ ] If fonts are broken, add `-s USE_FREETYPE=1` and re-add FreeType to deps
- [ ] Check if any UI assets use SVG (unlikely)

---

## Audio Effects

| Desktop Library | Status | WASM Replacement | Notes |
|-----------------|--------|------------------|-------|
| **SoundTouch** | ⚠️ Skipped | May need to re-add | Used for audio stretching (fast-forward, speed adjustments). |

**Current Issue:**
- ❌ Removed `SoundTouch::SoundTouchDLL` from util
- 🔧 **Risk:** Fast-forward and slow-motion may not work, audio will stutter

**Action Items:**
- [ ] Check if SoundTouch is WASM-compatible (it's pure C++, should be fine)
- [ ] Re-add SoundTouch to Emscripten build if needed
- [ ] Test fast-forward functionality

---

## Critical Missing Implementations

### 1. **Platform-Specific Code** (`platform_misc.h`)

**Current Status:**
- ✅ Windows: `platform_misc_win32.cpp`
- ✅ macOS: `platform_misc_mac.mm`
- ✅ Linux: `platform_misc_unix.cpp`
- ❌ **Web: No `platform_misc_web.cpp`**

**Functions Needed:**
```cpp
// Battery status, sleep prevention, etc.
bool Platform::IsBatteryPowered();
void Platform::PreventSleep();
void Platform::ResumeS sleep();
// etc.
```

**Action Items:**
- [ ] Create `src/util/platform_misc_web.cpp`
- [ ] Implement stubs or web equivalents (e.g., Wake Lock API for sleep prevention)

---

### 2. **Web-Specific Input Source**

**Current Status:**
- ❌ No `web_input_source.cpp` implementation

**Action Items:**
- [ ] Create `src/util/web_input_source.cpp`
- [ ] Use Emscripten's SDL2 gamepad support or raw Gamepad API
- [ ] Map keyboard keys to PS1 buttons

---

### 3. **Web-Specific Audio Stream**

**Current Status:**
- ❌ No `web_audio_stream.cpp` implementation

**Action Items:**
- [ ] Create `src/util/web_audio_stream.cpp`
- [ ] Use `SDL_OpenAudioDevice` (Emscripten's Web Audio backend)
- [ ] Or use `emscripten_set_main_loop` with audio callback

---

### 4. **File System Integration**

**How Users Load Games:**
1. **Browser File Upload:** `<input type="file">` → JavaScript `File` object
2. **Write to Emscripten FS:** `FS.writeFile('/roms/game.cue', data)`
3. **DuckStation loads:** `System::Boot('/roms/game.cue')`

**Implementation:**
- ✅ Emscripten MEMFS (`-s FORCE_FILESYSTEM=1`)
- 🔧 JavaScript bridge in `src/duckstation-web/web_host.cpp`

**Action Items:**
- [ ] Verify `_duckstation_load_bios()` writes BIOS to `/bios/`
- [ ] Verify `_duckstation_boot_game()` loads game from `/roms/`
- [ ] Test with real .cue/.bin file

---

## Build Configuration Audit

### Emscripten Flags in `src/duckstation-web/CMakeLists.txt`

**Current Flags:**
```cmake
"-s USE_WEBGL2=1"           # ✅ Correct
"-s FORCE_FILESYSTEM=1"     # ✅ Correct
"-s ALLOW_MEMORY_GROWTH=1"  # ✅ Correct
"-s INITIAL_MEMORY=256MB"   # ✅ Reasonable
```

**Missing Flags to Consider:**
```cmake
"-s USE_SDL=2"              # For input/audio via SDL2 emulation
"-s USE_ZLIB=1"             # For CHD/ZIP support
"-s USE_LIBPNG=1"           # If we load PNG textures
"-s USE_FREETYPE=1"         # If ImGui fonts break
"-s ASSERTIONS=1"           # Already set, good for debugging
```

**Action Items:**
- [ ] Add `-s USE_SDL=2` for audio and input
- [ ] Add `-s USE_ZLIB=1` to replace our dummy ZLIB target
- [ ] Test with minimal flags first, then add as needed

---

## Testing Checklist

### Phase 1: Basic Boot
- [ ] CMake configures without errors
- [ ] Ninja builds successfully
- [ ] `duckstation-web.js` and `.wasm` are generated
- [ ] Browser loads the WASM module without errors
- [ ] Console shows no critical errors

### Phase 2: BIOS Loading
- [ ] Upload BIOS file (`scph1001.bin`) via web UI
- [ ] BIOS is written to Emscripten FS at `/bios/`
- [ ] DuckStation recognizes BIOS (check console logs)

### Phase 3: Game Loading
- [ ] Upload game file (`.cue` + `.bin`) via web UI
- [ ] Files are written to `/roms/`
- [ ] DuckStation boots the game
- [ ] PS1 BIOS screen appears

### Phase 4: Graphics
- [ ] Game renders to canvas
- [ ] Frame rate is acceptable (30-60 FPS depending on CPU mode)
- [ ] No graphical corruption

### Phase 5: Audio
- [ ] Audio plays through browser
- [ ] Audio is in sync with video
- [ ] No crackling or stuttering

### Phase 6: Input
- [ ] Keyboard controls work (arrow keys, Z/X for O/X, etc.)
- [ ] Gamepad is detected
- [ ] Gamepad buttons map correctly to PS1 controller

### Phase 7: Advanced Features
- [ ] Save states work
- [ ] Memory cards work
- [ ] Multi-disc games can switch discs via M3U
- [ ] Fast-forward works (if SoundTouch is added)
- [ ] Post-processing shaders work (ReShade FX)

---

## Recommended Next Steps

1. **Finish the build first** - Get CMake + Ninja to complete without errors
2. **Test minimal boot** - Load BIOS, no game yet
3. **Add missing implementations incrementally:**
   - Web audio stream
   - Web input source
   - Platform misc web
4. **Test with a simple game** (e.g., Tekken 3, Ridge Racer)
5. **Add back libraries as needed** based on test failures

---

## References

- [Emscripten Porting Guide](https://emscripten.org/docs/porting/index.html)
- [Emscripten SDL2 Support](https://emscripten.org/docs/porting/multimedia_and_graphics/OpenGL-support.html)
- [Web Audio API Docs](https://developer.mozilla.org/en-US/docs/Web/API/Web_Audio_API)
- [Gamepad API Docs](https://developer.mozilla.org/en-US/docs/Web/API/Gamepad_API)
- [DuckStation Build Docs](docs/BUILD-WEB.md)

---

**Last Updated:** 2025-10-21
**Build Status:** 🔧 In Progress - Compilation phase (fixing architecture-specific code)

---

## Recent Build Fixes

### Architecture Detection (types.h)
- ✅ Added WebAssembly architecture detection
- ✅ Added `CPU_ARCH_WASM` flag
- ✅ Set `CPU_ARCH_STR` to "wasm32"
- ✅ Set `TARGET_OS_STR` to "Emscripten"

### Fast Jumps (fastjmp.h/cpp)
- ✅ Added WebAssembly support using standard C `setjmp`/`longjmp`
- ✅ Set buffer size to 64 bytes (standard jmp_buf size)
- ✅ Implemented wrapper functions for compatibility

### Crash Handler (crash_handler.cpp)
- ✅ Excluded backtrace.h for Emscripten builds
- ✅ Uses stub implementation (no crash dumps in browser)
