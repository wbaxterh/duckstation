# DuckStation Web Build - Troubleshooting Guide

Quick fixes for common build issues on Windows.

---

## ✅ The Easy Fix (99% of cases)

If you're getting `emcc: command not found` or any environment errors, just use the **complete build script**:

```powershell
.\scripts\build-web-complete.ps1
```

This handles everything automatically:
- ✅ Installs/updates emsdk
- ✅ Activates environment
- ✅ Builds the project
- ✅ Copies artifacts

Then start the server:
```powershell
.\scripts\serve-web.ps1
```

Open http://localhost:8080 and you're done!

---

## 🔧 Still Having Issues?

### Problem: "emcc: command not found" after running build-web.ps1

**Cause:** The emsdk environment variables aren't persisting.

**Solution:** Use the complete build script instead:
```powershell
.\scripts\build-web-complete.ps1
```

---

### Problem: "Execution of scripts is disabled on this system"

**Cause:** PowerShell script execution policy is restricted.

**Solution:** Run PowerShell as Administrator and execute:
```powershell
Set-ExecutionPolicy -ExecutionPolicy RemoteSigned -Scope CurrentUser
```

Then try again:
```powershell
.\scripts\build-web-complete.ps1
```

---

### Problem: Build fails with CMake errors

**Possible causes:**
1. Missing dependencies
2. Incompatible CMake version
3. Corrupted emsdk installation

**Solution 1 - Clean rebuild:**
```powershell
# Delete build directory
Remove-Item -Recurse -Force .\build-web

# Delete emsdk directory
Remove-Item -Recurse -Force .\.emsdk

# Start fresh
.\scripts\build-web-complete.ps1
```

**Solution 2 - Check CMake version:**
```powershell
cmake --version
```
You need CMake 3.20 or higher. Update if needed: https://cmake.org/download/

**Solution 3 - Check Ninja:**
```powershell
ninja --version
```
Install if missing: https://github.com/ninja-build/ninja/releases

---

### Problem: Build is extremely slow

**Cause:** Default build type is `RelWithDebInfo` (includes debug info).

**Solution:** Use Release build for faster compilation:
```powershell
.\scripts\build-web.ps1 Release
```

Or with complete build:
```powershell
.\scripts\build-web-complete.ps1 Release
```

---

### Problem: Python errors during emsdk setup

**Cause:** Emsdk comes with its own Python, but there might be conflicts.

**Solution 1 - Use emsdk's Python:**
The setup script uses emsdk's bundled Python automatically. Make sure you're running:
```powershell
.\scripts\setup-emsdk.ps1
```

**Solution 2 - Check for PATH conflicts:**
```powershell
$env:PATH
```
If you see multiple Python installations, temporarily remove them from PATH during build.

---

### Problem: "web-dist/duckstation is empty after build"

**Cause:** Build succeeded but artifacts weren't copied.

**Check build output:**
```powershell
dir .\build-web\bin
```

If you see `duckstation-web.js` and `duckstation-web.wasm` there, manually copy:
```powershell
Copy-Item .\build-web\bin\duckstation-web.* .\web-dist\duckstation\
```

---

### Problem: Server won't start (port 8080 in use)

**Cause:** Another process is using port 8080.

**Solution - Use different port:**
```powershell
.\scripts\serve-web.ps1 -Port 8081
```

Then open http://localhost:8081

**Solution - Find what's using port 8080:**
```powershell
netstat -ano | findstr :8080
```
Kill the process or use a different port.

---

### Problem: Browser shows blank screen

**Possible causes:**
1. WASM files not loaded
2. JavaScript errors
3. CORS/COOP/COEP issues

**Solution:**
1. Open browser DevTools (F12)
2. Check Console tab for errors
3. Check Network tab - make sure `.wasm` and `.js` files load (status 200)

Common fixes:
- Make sure you're using the dev server (not opening `index.html` directly)
- Clear browser cache (Ctrl+Shift+R)
- Try different browser (Chrome/Edge recommended)

---

### Problem: Game won't boot in browser

**Check these:**

1. **BIOS file format:** Must be `.bin` (not `.exe` or compressed)
2. **Game file format:** Supported: `.cue`, `.bin`, `.iso`, `.chd`
3. **File paths:** Make sure files uploaded successfully (check console)

**Debug in browser console (F12):**
```javascript
// Check if files were mounted
Module.FS.readdir('/bios')
Module.FS.readdir('/roms')
```

---

### Problem: Performance is terrible

**Expected behavior:**
- Web build uses CachedInterpreter (not JIT)
- Expect 50-70% of native speed
- Some games will be slow or unplayable

**Optimizations:**
1. Use Release build: `.\scripts\build-web.ps1 Release`
2. Try lighter/2D games first
3. Reduce resolution in emulator settings
4. Close other browser tabs

**Note:** This is a WASM limitation. JIT recompilers can't run in browsers.

---

## 🆘 Still Stuck?

### Collect debug info:

```powershell
# Check versions
emcc --version
cmake --version
ninja --version

# Check emsdk installation
dir .\.emsdk\upstream\emscripten

# Check build output
dir .\build-web\bin

# Check web-dist
dir .\web-dist\duckstation
```

### Clean slate (nuclear option):

```powershell
# Delete everything
Remove-Item -Recurse -Force .\build-web
Remove-Item -Recurse -Force .\.emsdk
Remove-Item -Recurse -Force .\web-dist\duckstation\*

# Start completely fresh
.\scripts\build-web-complete.ps1
```

---

## 📋 Checklist for Success

Before asking for help, verify:

- [ ] Ran `.\scripts\build-web-complete.ps1` (not just `build-web.ps1`)
- [ ] PowerShell execution policy allows scripts
- [ ] CMake 3.20+ installed
- [ ] Ninja build system installed
- [ ] No other processes using port 8080
- [ ] Using Chrome or Edge browser (not Firefox/Safari)
- [ ] Checked browser console (F12) for errors
- [ ] Tried a clean rebuild (deleted `build-web/` and `.emsdk/`)

---

## 🎯 Quick Command Reference

| Task | Command |
|------|---------|
| First time build | `.\scripts\build-web-complete.ps1` |
| Rebuild after changes | `.\scripts\build-web.ps1` |
| Start dev server | `.\scripts\serve-web.ps1` |
| Custom port | `.\scripts\serve-web.ps1 -Port 8081` |
| Release build | `.\scripts\build-web.ps1 Release` |
| Clean build | Delete `build-web/`, then rebuild |
| Reset everything | Delete `build-web/` and `.emsdk/`, then `build-web-complete.ps1` |

---

**Happy Emulating! 🦆🎮**
