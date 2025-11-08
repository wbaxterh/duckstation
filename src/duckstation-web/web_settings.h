// SPDX-FileCopyrightText: 2019-2024 Connor McLaughlin <stenzek@gmail.com>
// SPDX-License-Identifier: CC-BY-NC-ND-4.0

#pragma once

#include "util/ini_settings_interface.h"

#include "common/small_string.h"
#include "common/types.h"

#include <mutex>
#include <string>
#include <vector>

namespace WebSettings {

// Global settings instance - direct access, no virtual functions
extern INISettingsInterface g_settings;
extern std::mutex g_settings_mutex;

// CRITICAL: Direct accessors that bypass ALL virtual functions
// We're hardcoding common values to avoid vtable entirely
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
  if (std::strcmp(section, "BIOS") == 0) {
    if (std::strcmp(key, "PatchFastBoot") == 0) return true;
  }
  if (std::strcmp(section, "Audio") == 0) {
    if (std::strcmp(key, "OutputMuted") == 0) return true;
  }

  return default_value;
}

inline s32 GetInt(const char* section, const char* key, s32 default_value = 0)
{
  // HARDCODED - bypass vtable
  return default_value;
}

inline u32 GetUInt(const char* section, const char* key, u32 default_value = 0)
{
  // HARDCODED - bypass vtable
  if (std::strcmp(section, "Audio") == 0 && std::strcmp(key, "OutputVolume") == 0)
    return 100;
  return default_value;
}

inline float GetFloat(const char* section, const char* key, float default_value = 0.0f)
{
  // HARDCODED - bypass vtable
  return default_value;
}

inline double GetDouble(const char* section, const char* key, double default_value = 0.0)
{
  // HARDCODED - bypass vtable
  return default_value;
}

inline std::string GetString(const char* section, const char* key, const char* default_value = "")
{
  // HARDCODED VALUES - bypassing vtable completely
  if (std::strcmp(section, "CPU") == 0 && std::strcmp(key, "ExecutionMode") == 0)
    return "CachedInterpreter";
  if (std::strcmp(section, "GPU") == 0 && std::strcmp(key, "Renderer") == 0)
    return "Software";
  if (std::strcmp(section, "Audio") == 0 && std::strcmp(key, "Backend") == 0)
    return "Null";
  if (std::strcmp(section, "Console") == 0 && std::strcmp(key, "Region") == 0)
    return "Auto";

  return std::string(default_value);
}

inline SmallString GetSmallString(const char* section, const char* key, const char* default_value = "")
{
  // HARDCODED - bypass vtable
  return SmallString(default_value);
}

inline TinyString GetTinyString(const char* section, const char* key, const char* default_value = "")
{
  // HARDCODED - bypass vtable
  return TinyString(default_value);
}

inline std::vector<std::string> GetStringList(const char* section, const char* key)
{
  // HARDCODED - bypass vtable
  return std::vector<std::string>();
}

// Setter methods - all no-ops to bypass vtable
inline void SetBool(const char* section, const char* key, bool value)
{
  // NO-OP - bypass vtable
}

inline void SetInt(const char* section, const char* key, s32 value)
{
  // NO-OP - bypass vtable
}

inline void SetUInt(const char* section, const char* key, u32 value)
{
  // NO-OP - bypass vtable
}

inline void SetFloat(const char* section, const char* key, float value)
{
  // NO-OP - bypass vtable
}

inline void SetString(const char* section, const char* key, const char* value)
{
  // NO-OP - bypass vtable
}

inline void SetStringList(const char* section, const char* key, const std::vector<std::string>& values)
{
  // NO-OP - bypass vtable
}

inline bool AddToStringList(const char* section, const char* key, const char* value)
{
  // NO-OP - bypass vtable
  return false;
}

inline bool RemoveFromStringList(const char* section, const char* key, const char* value)
{
  // NO-OP - bypass vtable
  return false;
}

inline bool ContainsValue(const char* section, const char* key)
{
  // NO-OP - bypass vtable
  return false;
}

inline void DeleteValue(const char* section, const char* key)
{
  // NO-OP - bypass vtable
}

// Get direct access to settings interface (for Host::Internal functions)
inline INISettingsInterface* GetInterface()
{
  return &g_settings;
}

} // namespace WebSettings
