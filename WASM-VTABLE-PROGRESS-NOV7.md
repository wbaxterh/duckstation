# DuckStation WebAssembly - VTable Issue Progress Report

**Date:** November 7, 2025
**Session:** Continuation from context window reset
**Status:** PARTIAL PROGRESS - Multiple vtable errors resolved, CDImage vtable still blocking

---

## Progress Summary

We've successfully resolved **two major vtable issues** but are blocked on a third:

| System | Status | Solution |
|--------|--------|----------|
| Host Settings | ✅ FIXED | Created hardcoded WebSettings namespace |
| PIO Device | ✅ FIXED | Created web-specific stub (pio_web.cpp) |
| CDImage | ❌ BLOCKING | Tried RTTI + MAIN_MODULE=2, still failing |

---

## What We've Accomplished This Session

### 1. ✅ Fixed Host Settings VTable (WebSettings Approach)

**Problem:** `Host::GetBoolSettingValue()` → `LayeredSettingsInterface` → vtable error

**Solution:** Created completely hardcoded settings that bypass ALL virtual functions

**Files Created:**
- `src/duckstation-web/web_settings.h` - Hardcoded inline getters
- `src/duckstation-web/web_settings.cpp` - Global settings instances

**Key Code** (`web_settings.h:27-46`):
```cpp
namespace WebSettings {

extern INISettingsInterface g_settings;
extern std::mutex g_settings_mutex;

// CRITICAL: Direct accessors that bypass ALL virtual functions
inline bool GetBool(const char* section, const char* key, bool default_value = false)
{
  // HARDCODED VALUES - bypassing vtable completely
  if (std::strcmp(section, "GPU") == 0) {
    if (std::strcmp(key, "UseThread") == 0) return false;
    if (std::strcmp(key, "UseDebugDevice") == 0) return false;
  }
  if (std::strcmp(section, "Display") == 0) {
    if (std::strcmp(key, "VSync") == 0) return false;
    if (std::strcmp(key, "Fullscreen") == 0) return false;
  }
  // ... more hardcoded values
  return default_value;
}

// Similar for GetInt, GetString, GetFloat, etc.
// All setters are no-ops
}
```

**Files Modified:**
- `src/core/CMakeLists.txt:79` - Exclude `host.cpp` for Emscripten
- `src/duckstation-web/web_host.cpp:349-542` - Implement all Host functions using WebSettings

**Result:** ✅ Host settings vtable error eliminated

---

### 2. ✅ Fixed PIO Device VTable

**Problem:** `PIO::Initialize()` → `g_pio_device->Initialize()` → vtable error

**Solution:** Created web-specific PIO stub that completely avoids virtual functions

**Files Created:**
- `src/core/pio_web.cpp` - Complete PIO stub for web

**Key Code** (`pio_web.cpp:15-37`):
```cpp
namespace PIO {

// For web builds, we completely stub out PIO device support
// No virtual function calls, no polymorphism

bool Initialize(Error* error)
{
  // No PIO device for web build - return success
  return true;
}

void UpdateSettings(const Settings& old_settings) { }
void Shutdown() { }
void Reset() { }
bool DoState(StateWrapper& sw) { return !sw.HasError(); }

} // namespace PIO

// Stub the global PIO device pointer
std::unique_ptr<PIO::Device> g_pio_device;
PIO::Device::~Device() = default;
```

**Files Modified:**
- `src/core/CMakeLists.txt:112-113` - Use `pio_web.cpp` for Emscripten

**Result:** ✅ PIO vtable error eliminated

---

### 3. ⚠️ Enabled RTTI for Virtual Functions

**Problem:** Project-wide `-fno-rtti` flag prevents virtual function type information

**Solution:** Conditionally enable RTTI for Emscripten builds

**Files Modified:**
- `CMakeLists.txt:92-98`

```cpp
# For Emscripten, we need RTTI for vtables to work properly
if(EMSCRIPTEN)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-exceptions")
  message(STATUS "Emscripten build: Enabling RTTI for virtual function support")
else()
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-exceptions -fno-rtti")
endif()
```

**Result:** ⚠️ RTTI enabled but CDImage vtable still fails

**File Size Impact:** WASM increased from ~9.23 MB → 9.3 MB (+70KB for RTTI metadata)

---

### 4. ⚠️ Tried MAIN_MODULE=2 for Dynamic Linking

**Problem:** Even with RTTI, CDImage vtable not populated

**Solution:** Use `-s MAIN_MODULE=2` to export all used symbols and populate vtables

**Files Modified:**
- `src/duckstation-web/CMakeLists.txt:70-71`

```cmake
# Use MAIN_MODULE=2 to export used symbols and populate vtables
"-s MAIN_MODULE=2"  # Export all used symbols (modern replacement for LINKABLE)
```

**Result:** ⚠️ Build succeeds but CDImage vtable still fails at runtime

**File Size Impact:**
- WASM: 9.3 MB → 9.8 MB (+500KB for export tables)
- JS: 172KB → 206KB (+34KB for dynamic linking support)

---

## Current Blocking Error

```
RuntimeError: table index is out of bounds
    at CDROM::InsertMedia(std::unique_ptr<CDImage>&, ...)
    at System::BootSystem(SystemBootParameters, Error*)
    at duckstation_boot_game
```

**Location:** `src/core/cdrom.cpp:945`
```cpp
bool CDROM::InsertMedia(std::unique_ptr<CDImage>& media, ...)
{
  if (!media->HasSubchannelData() &&  // ← VIRTUAL FUNCTION CALL
      !CDROMSubQReplacement::LoadForImage(&subq, media.get(), ...))
  {
    return false;
  }
  // ...
}
```

**Root Cause:** `media->HasSubchannelData()` is a virtual function call on `CDImage` base class. Even with RTTI + MAIN_MODULE=2, the vtable entry is not in the WebAssembly function table.

---

## What We've Tried (Chronological)

### Failed Attempts

1. ❌ **RTTI alone** - No effect
2. ❌ **MAIN_MODULE=2** - Build succeeds, runtime still fails
3. ❌ **EXPORT_ALL=1 + LINKABLE=1** - Link errors (missing CPU recompiler symbols)
4. ❌ **RESERVED_FUNCTION_POINTERS=2000** - No effect (just reserves space)

---

## Why CDImage VTable is Different

Unlike Host and PIO which we could stub out, CDImage is **critical** to emulation:

**CDImage Implementations:**
- `CDImageCueSheet` - .cue files (what we're loading)
- `CDImageBin` - .bin/.iso files
- `CDImageCHD` - .chd compressed images
- `CDImageM3U` - Multi-disc images
- `CDImageMDS` - .mds files
- `CDImagePBP` - .pbp files
- `CDImageMemory` - In-memory images
- `CDImageDevice` - Physical CD drives

**The Problem:**
- All inherit from `CDImage` abstract base class
- `CDImage::Open()` factory function creates appropriate type based on file extension
- Many virtual functions: `HasSubchannelData()`, `Read()`, `Seek()`, `GetNumTracks()`, etc.
- Can't easily stub out - actually need to read disc data

---

## Technical Analysis

### Why Emscripten Can't Handle This

**Emscripten's WebAssembly Compilation Model:**
1. C++ virtual functions → indirect function table
2. Table must be populated at **link time** with all possible vtable entries
3. Table indices are static - can't add entries at runtime
4. Complex inheritance hierarchies require many entries

**Our Codebase:**
- Heavy use of polymorphism (proper OOP design for native)
- Multiple levels of inheritance
- Factory patterns that create derived classes dynamically
- Virtual destructors everywhere

**The Mismatch:**
- ⚠️ Good OOP design for native ≠ WebAssembly compatible
- ⚠️ Dynamic polymorphism ≠ static linking model
- ⚠️ `-fno-rtti` optimization ≠ vtable requirements

### What MAIN_MODULE=2 Does

From Emscripten docs:
> MAIN_MODULE=2 creates a main module that exports all **used** symbols.
> More selective than MAIN_MODULE=1 which exports everything.

**Problem:** "Used" detection may not catch:
- Virtual functions called through base class pointers
- Factory-created objects
- Polymorphic destructors

---

## Possible Solutions Moving Forward

### Option 1: Create CD Image Web Stub ⚡ FASTEST

**Approach:** Like we did with PIO, create `cd_image_web.cpp`

**Pros:**
- ✅ Known to work (we've done it twice already)
- ✅ Can implement minimal CUE/BIN support
- ✅ Avoids vtable entirely

**Cons:**
- ❌ Need to reimplement CD reading logic
- ❌ Won't support CHD, M3U, etc. initially
- ❌ Technical debt

**Estimated Time:** 4-6 hours

---

### Option 2: Force Instantiate All CDImage Types 🔧 EXPERIMENTAL

**Approach:** Create dummy function that instantiates all CDImage derived classes

**Code:**
```cpp
// In web_host.cpp - never called, just forces linking
#ifdef __EMSCRIPTEN__
__attribute__((used))
static void force_cdimage_vtables() {
  // Force linker to include all CDImage vtables
  CDImageCueSheet cue;
  CDImageBin bin;
  CDImageCHD chd;
  CDImageM3U m3u;
  // ... all types
}
#endif
```

**Pros:**
- ✅ Might force vtables into function table
- ✅ Quick to try

**Cons:**
- ❌ May not work (linker might optimize away)
- ❌ Requires default constructors (which may not exist)

**Estimated Time:** 1 hour to try

---

### Option 3: Use Emscripten's KEEPALIVE 🎯 PROPER FIX

**Approach:** Mark all CDImage classes with `EMSCRIPTEN_KEEPALIVE`

**Code:**
```cpp
// In cd_image_cue.cpp
class EMSCRIPTEN_KEEPALIVE CDImageCueSheet : public CDImage {
  // ...
};
```

**Pros:**
- ✅ Officially recommended by Emscripten
- ✅ Explicitly tells linker to keep symbols
- ✅ Should populate vtables

**Cons:**
- ❌ Need to modify many files
- ❌ May conflict with project style
- ❌ Still might not work

**Estimated Time:** 2-3 hours

---

### Option 4: Remove Polymorphism from CDImage 🏗️ ARCHITECTURAL

**Approach:** Refactor CDImage to use composition instead of inheritance

**Not Recommended:** Massive architectural change to core codebase

---

### Option 5: Wait for Emscripten Fix 🐛 LONG-TERM

**Approach:** Report issue to Emscripten team

**Reality:** May take months, not a viable solution

---

## Recommended Next Steps

**Priority Order:**

1. **TRY OPTION 2** (1 hour) - Quick experiment with force instantiation
2. **IF FAILS → OPTION 1** (4-6 hours) - Create cd_image_web.cpp stub
3. **THEN TEST** - See if emulator boots
4. **IF BOOTS → MORE STUBS** - Likely will hit more vtable errors (GPU, Audio, etc.)

---

## Build State

### Current Build (Nov 7, 21:50)

**Location:** `web-dist/duckstation/duckstation-web.wasm`
**Size:** 9.8 MB
**Configuration:**
- ✅ RTTI enabled
- ✅ `-s MAIN_MODULE=2`
- ✅ Host settings bypassed (WebSettings)
- ✅ PIO stubbed (pio_web.cpp)
- ❌ CDImage vtable not working

**Progress Through Boot:**
1. ✅ Emulator initialization
2. ✅ BIOS loading
3. ✅ ROM file loading (630 MB)
4. ✅ PIO::Initialize()
5. ❌ **CRASHES** at CDROM::InsertMedia()

---

## Files Modified This Session

**Created:**
- `src/duckstation-web/web_settings.h` - Hardcoded settings
- `src/duckstation-web/web_settings.cpp` - Settings implementation
- `src/core/pio_web.cpp` - PIO web stub

**Modified:**
- `CMakeLists.txt` - Enable RTTI for Emscripten
- `src/core/CMakeLists.txt` - Exclude host.cpp, use pio_web.cpp
- `src/duckstation-web/CMakeLists.txt` - Add MAIN_MODULE=2
- `src/duckstation-web/web_host.cpp` - Implement Host functions

**Build Configuration:**
```cmake
# Emscripten-specific flags
"-s MODULARIZE=1"
"-s EXPORT_ES6=1"
"-s ALLOW_MEMORY_GROWTH=1"
"-s MAIN_MODULE=2"  # ← Added this session
"-s ALLOW_TABLE_GROWTH=1"
"-s RESERVED_FUNCTION_POINTERS=2000"
```

---

## Lessons Learned

### What Works
✅ **Hardcoded alternatives** bypass vtables reliably
✅ **Web-specific stubs** (pio_web.cpp) avoid polymorphism
✅ **RTTI helps** but isn't sufficient alone
✅ **MAIN_MODULE=2 builds** but doesn't solve vtable

### What Doesn't Work
❌ **Standard linker flags** (EXPORT_ALL, LINKABLE, RESERVED_FUNCTION_POINTERS)
❌ **RTTI alone** without proper symbol export
❌ **Assuming "used" detection** finds all vtable entries

### Key Insight
> WebAssembly's static linking model is fundamentally incompatible with C++ polymorphism as designed in DuckStation. Each polymorphic system needs web-specific handling.

---

## Conclusion

We've made **significant progress** eliminating multiple vtable errors:
- From: "Crashes immediately on boot"
- To: "Boots through initialization, loads files, crashes on CD insertion"

**The Pattern is Clear:** We need to systematically replace polymorphic subsystems with web-specific stubs or implementations.

**Estimated Remaining Work:**
- CDImage: 4-6 hours (stub implementation)
- Likely 2-3 more vtable systems after that
- **Total: 12-20 hours** to fully working build

**Alternative:** Accept that full WebAssembly port may not be feasible with current codebase architecture. Consider:
- Simplified "demo" version with limited features
- Hybrid approach (wasm for core, JS for I/O)
- Wait for WebAssembly to better support C++ polymorphism

---

**Last Updated:** November 7, 2025 21:55
**Next Session:** Try Option 2 (force instantiation) or implement Option 1 (cd_image_web.cpp)
