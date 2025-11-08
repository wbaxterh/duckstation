# DuckStation WebAssembly - Quick Start Guide

## 🚀 Start the Emulator (3 Steps)

### 1. Build (First Time Only)

```powershell
cd C:\Users\wesle\Documents\git\duckstation
.\scripts\build-web.ps1 Release
```

**Wait:** ~2-5 minutes for first build

### 2. Start Server

```powershell
.\scripts\serve-web.ps1
```

**Keep this window open!**

### 3. Open Browser

Go to: **http://localhost:8080/play.html**

---

## 🎮 Load Tony Hawk's Pro Skater

1. **Select BIOS:** `roms/SCPH1001.BIN` (512 KB)
2. **Select CUE:** `roms/thps.cue`
3. **Select ROM:** `roms/thps.bin` (630 MB - takes ~15 seconds to load)
4. **Click:** "Start Game"

---

## 🛑 Stop Server

Press `Ctrl+C` in the PowerShell window

---

## 🔄 Rebuild After Code Changes

```powershell
.\scripts\build-web.ps1 Release
```

The server will automatically serve the new files.

---

## 📚 Full Documentation

- **[WASM-DEBUGGING-AND-RUNNING.md](WASM-DEBUGGING-AND-RUNNING.md)** - Complete guide
- **[WASM-PORT-SUMMARY.md](WASM-PORT-SUMMARY.md)** - Technical implementation details

---

## ⚠️ Current Status

**Threading:** ✅ Fixed (disabled for WebAssembly)
**Logging:** 🚧 In Progress (crashes on boot)
**Latest Build:** `duckstation-web-WORKING.wasm` (9.5 MB)

**Known Issue:** Memory access crash in logging system during CDROM initialization

---

## 🐛 Quick Troubleshooting

**Problem:** "404 Not Found"
```powershell
# Restart server
Ctrl+C
.\scripts\serve-web.ps1
```

**Problem:** Browser shows old version
```
# Hard refresh: Ctrl+Shift+R
# Or close browser completely and reopen
```

**Problem:** Build fails
```powershell
# Clean and rebuild
Remove-Item -Recurse -Force build-web
.\scripts\build-web.ps1 Release
```

---

**Last Updated:** October 27, 2025
