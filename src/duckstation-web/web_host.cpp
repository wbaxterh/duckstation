// SPDX-FileCopyrightText: 2025 DuckStation Web Contributors
// SPDX-License-Identifier: CC-BY-NC-ND-4.0

// ==============================================================================
// DuckStation Web Frontend
// ==============================================================================
// This is a minimal web-based frontend for DuckStation, designed to run in
// modern browsers via WebAssembly/Emscripten. It's based on the "mini" frontend
// but adapted for browser constraints.
//
// Key differences from native builds:
// - Uses Cached Interpreter instead of JIT recompiler (WASM limitation)
// - Single-threaded emulation loop (no CPU/GPU thread separation initially)
// - Browser-based file I/O via Emscripten FS
// - Canvas-based rendering (WebGL)
// - Web Audio API for sound
// ==============================================================================

#include "scmversion/scmversion.h"

#include "core/achievements.h"
#include "core/fullscreen_ui.h"
#include "core/game_list.h"
#include "core/gpu_thread.h"
#include "core/host.h"
#include "core/settings.h"
#include "core/system.h"
#include "core/system_private.h"
#include "core/gpu.h"
#include "core/gpu_backend.h"

#include "util/gpu_device.h"
#include "util/imgui_fullscreen.h"
#include "util/imgui_manager.h"
#include "util/ini_settings_interface.h"
#include "util/input_manager.h"

#include "common/assert.h"
#include "common/error.h"
#include "common/file_system.h"
#include "common/log.h"
#include "common/path.h"
#include "common/string_util.h"

#include "fmt/format.h"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/html5.h>
#include "web_settings.h"
#endif

#include <cstdio>
#include <string>
#include <optional>

// Use existing log channel
LOG_CHANNEL(Host);

namespace WebHost {

static constexpr u32 SETTINGS_VERSION = 1;

// Global state
struct WebHostState
{
  INISettingsInterface settings_interface;
  std::string bios_path;
  std::string game_path;
  bool initialized = false;
  bool running = false;
};

static WebHostState s_state;

// Forward declarations
static bool InitializeConfig();
static void SetDefaultSettings(SettingsInterface& si);
static bool InitializeSystem();
static void ShutdownSystem();
static void MainLoopIteration();
static void SetCriticalFolders();

// ==============================================================================
// Initialization
// ==============================================================================

void SetCriticalFolders()
{
  // For web builds, use virtual filesystem paths
  EmuFolders::AppRoot = "/app";
  EmuFolders::DataRoot = "/data";
  EmuFolders::Resources = "/app/resources";

  // Create virtual directories via Emscripten FS (done from JavaScript)
  INFO_LOG("AppRoot: {}", EmuFolders::AppRoot);
  INFO_LOG("DataRoot: {}", EmuFolders::DataRoot);
  INFO_LOG("Resources: {}", EmuFolders::Resources);
}

bool InitializeConfig()
{
  SetCriticalFolders();

  std::string settings_path = Path::Combine(EmuFolders::DataRoot, "settings.ini");
  INFO_LOG("Loading config from {}", settings_path);

#ifdef __EMSCRIPTEN__
  // Use WebSettings for Emscripten builds
  WebSettings::g_settings.SetPath(std::move(settings_path));
  Host::Internal::SetBaseSettingsLayer(&WebSettings::g_settings);

  if (!WebSettings::g_settings.Load())
  {
    INFO_LOG("No existing config, creating default settings");
    WebSettings::g_settings.SetUIntValue("Main", "SettingsVersion", SETTINGS_VERSION);
    SetDefaultSettings(WebSettings::g_settings);

    Error error;
    if (!WebSettings::g_settings.Save(&error))
    {
      ERROR_LOG("Failed to save default config: {}", error.GetDescription());
      return false;
    }
  }

  EmuFolders::LoadConfig(WebSettings::g_settings);
#else
  // Use local state for non-Emscripten builds
  s_state.settings_interface.SetPath(std::move(settings_path));
  Host::Internal::SetBaseSettingsLayer(&s_state.settings_interface);

  if (!s_state.settings_interface.Load())
  {
    INFO_LOG("No existing config, creating default settings");
    s_state.settings_interface.SetUIntValue("Main", "SettingsVersion", SETTINGS_VERSION);
    SetDefaultSettings(s_state.settings_interface);

    Error error;
    if (!s_state.settings_interface.Save(&error))
    {
      ERROR_LOG("Failed to save default config: {}", error.GetDescription());
      return false;
    }
  }

  EmuFolders::LoadConfig(s_state.settings_interface);
#endif

  EmuFolders::EnsureFoldersExist();

  return true;
}

void SetDefaultSettings(SettingsInterface& si)
{
  // Critical: Force cached interpreter mode for WASM (no JIT support)
  si.SetStringValue("CPU", "ExecutionMode", "CachedInterpreter");

  // Use software or WebGL renderer
  si.SetStringValue("GPU", "Renderer", "Software");

  // Disable features that won't work in browser
  si.SetBoolValue("GPU", "UseThread", false);  // Single-threaded for now
  si.SetBoolValue("Display", "VSync", false);   // Browser handles vsync

  // Audio settings
  si.SetStringValue("Audio", "Backend", "Null");  // Will be updated to WebAudio
  si.SetUIntValue("Audio", "OutputVolume", 100);

  // Basic system defaults
  si.SetStringValue("Console", "Region", "Auto");
  si.SetBoolValue("BIOS", "PatchFastBoot", true);

  System::SetDefaultSettings(si);
  EmuFolders::SetDefaults();
  EmuFolders::Save(si);
}

bool InitializeSystem()
{
  Error error;

  if (!System::ProcessStartup(&error))
  {
    ERROR_LOG("System::ProcessStartup failed: {}", error.GetDescription());
    return false;
  }

  if (!System::CPUThreadInitialize(&error, 0))  // 0 async worker threads for web
  {
    ERROR_LOG("System::CPUThreadInitialize failed: {}", error.GetDescription());
    return false;
  }

  INFO_LOG("DuckStation Web initialized successfully");
  s_state.initialized = true;
  return true;
}

void ShutdownSystem()
{
  if (System::IsValid())
    System::ShutdownSystem(false);

  if (s_state.initialized)
  {
    System::CPUThreadShutdown();
    System::ProcessShutdown();
    s_state.initialized = false;
  }
}

// ==============================================================================
// Main Loop
// ==============================================================================

void MainLoopIteration()
{
  if (!s_state.running)
    return;

  if (System::IsRunning())
  {
    System::Execute();
  }
}

} // namespace WebHost

// ==============================================================================
// Host Interface Implementation
// ==============================================================================

void Host::ReportFatalError(std::string_view title, std::string_view message)
{
  ERROR_LOG("FATAL ERROR: {}: {}", title, message);
  std::fprintf(stderr, "FATAL: %.*s: %.*s\n",
    static_cast<int>(title.size()), title.data(),
    static_cast<int>(message.size()), message.data());
}

void Host::ReportErrorAsync(std::string_view title, std::string_view message)
{
  ERROR_LOG("ERROR: {}: {}", title, message);
  std::fprintf(stderr, "ERROR: %.*s: %.*s\n",
    static_cast<int>(title.size()), title.data(),
    static_cast<int>(message.size()), message.data());
}

void Host::ReportDebuggerMessage(std::string_view message)
{
  INFO_LOG("Debugger: {}", message);
}

std::span<const std::pair<const char*, const char*>> Host::GetAvailableLanguageList()
{
  return {};
}

const char* Host::GetLanguageName(std::string_view language_code)
{
  return "English";
}

bool Host::ChangeLanguage(const char* new_language)
{
  return false;
}

void Host::AddFixedInputBindings(const SettingsInterface& si) {}

void Host::OnInputDeviceConnected(InputBindingKey key, std::string_view identifier, std::string_view device_name) {}
void Host::OnInputDeviceDisconnected(InputBindingKey key, std::string_view identifier) {}

s32 Host::Internal::GetTranslatedStringImpl(std::string_view context, std::string_view msg,
                                            std::string_view disambiguation, char* tbuf, size_t tbuf_space)
{
  if (msg.size() > tbuf_space)
    return -1;
  if (msg.empty())
    return 0;

  std::memcpy(tbuf, msg.data(), msg.size());
  return static_cast<s32>(msg.size());
}

std::string Host::TranslatePluralToString(const char* context, const char* msg, const char* disambiguation, int count)
{
  std::string ret(msg);
  ret.replace(ret.find("%n"), 2, std::to_string(count));
  return ret;
}

SmallString Host::TranslatePluralToSmallString(const char* context, const char* msg, const char* disambiguation, int count)
{
  SmallString ret(msg);
  ret.replace("%n", TinyString::from_format("{}", count));
  return ret;
}

bool Host::ResourceFileExists(std::string_view filename, bool allow_override)
{
  const std::string path = Path::Combine(EmuFolders::Resources, filename);
  return FileSystem::FileExists(path.c_str());
}

std::optional<DynamicHeapArray<u8>> Host::ReadResourceFile(std::string_view filename, bool allow_override, Error* error)
{
  const std::string path = Path::Combine(EmuFolders::Resources, filename);
  return FileSystem::ReadBinaryFile(path.c_str(), error);
}

std::optional<std::string> Host::ReadResourceFileToString(std::string_view filename, bool allow_override, Error* error)
{
  const std::string path = Path::Combine(EmuFolders::Resources, filename);
  return FileSystem::ReadFileToString(path.c_str(), error);
}

std::optional<std::time_t> Host::GetResourceFileTimestamp(std::string_view filename, bool allow_override)
{
  const std::string path = Path::Combine(EmuFolders::Resources, filename);
  FILESYSTEM_STAT_DATA sd;
  if (!FileSystem::StatFile(path.c_str(), &sd))
    return std::nullopt;
  return sd.ModificationTime;
}

void Host::LoadSettings(const SettingsInterface& si, std::unique_lock<std::mutex>& lock) {}
void Host::CheckForSettingsChanges(const Settings& old_settings) {}

void Host::CommitBaseSettingChanges()
{
  auto lock = Host::GetSettingsLock();
  Error error;
#ifdef __EMSCRIPTEN__
  if (!WebSettings::g_settings.Save(&error))
    ERROR_LOG("Failed to save settings: {}", error.GetDescription());
#else
  if (!WebHost::s_state.settings_interface.Save(&error))
    ERROR_LOG("Failed to save settings: {}", error.GetDescription());
#endif
}

// ==============================================================================
// Host Setting Getters - Using WebSettings to bypass vtable
// ==============================================================================

#ifdef __EMSCRIPTEN__

// Settings lock and interface access
std::unique_lock<std::mutex> Host::GetSettingsLock()
{
  return std::unique_lock(WebSettings::g_settings_mutex);
}

SettingsInterface* Host::GetSettingsInterface()
{
  return &WebSettings::g_settings;
}

// Base setting getters - direct access to INI file
std::string Host::GetBaseStringSettingValue(const char* section, const char* key, const char* default_value)
{
  return WebSettings::GetString(section, key, default_value);
}

SmallString Host::GetBaseSmallStringSettingValue(const char* section, const char* key, const char* default_value)
{
  return WebSettings::GetSmallString(section, key, default_value);
}

TinyString Host::GetBaseTinyStringSettingValue(const char* section, const char* key, const char* default_value)
{
  return WebSettings::GetTinyString(section, key, default_value);
}

bool Host::GetBaseBoolSettingValue(const char* section, const char* key, bool default_value)
{
  return WebSettings::GetBool(section, key, default_value);
}

s32 Host::GetBaseIntSettingValue(const char* section, const char* key, s32 default_value)
{
  return WebSettings::GetInt(section, key, default_value);
}

u32 Host::GetBaseUIntSettingValue(const char* section, const char* key, u32 default_value)
{
  return WebSettings::GetUInt(section, key, default_value);
}

float Host::GetBaseFloatSettingValue(const char* section, const char* key, float default_value)
{
  return WebSettings::GetFloat(section, key, default_value);
}

double Host::GetBaseDoubleSettingValue(const char* section, const char* key, double default_value)
{
  return WebSettings::GetDouble(section, key, default_value);
}

std::vector<std::string> Host::GetBaseStringListSetting(const char* section, const char* key)
{
  return WebSettings::GetStringList(section, key);
}

// Layered setting getters - same as base for WebAssembly (no game-specific overrides)
std::string Host::GetStringSettingValue(const char* section, const char* key, const char* default_value)
{
  return WebSettings::GetString(section, key, default_value);
}

SmallString Host::GetSmallStringSettingValue(const char* section, const char* key, const char* default_value)
{
  return WebSettings::GetSmallString(section, key, default_value);
}

TinyString Host::GetTinyStringSettingValue(const char* section, const char* key, const char* default_value)
{
  return WebSettings::GetTinyString(section, key, default_value);
}

bool Host::GetBoolSettingValue(const char* section, const char* key, bool default_value)
{
  return WebSettings::GetBool(section, key, default_value);
}

s32 Host::GetIntSettingValue(const char* section, const char* key, s32 default_value)
{
  return WebSettings::GetInt(section, key, default_value);
}

u32 Host::GetUIntSettingValue(const char* section, const char* key, u32 default_value)
{
  return WebSettings::GetUInt(section, key, default_value);
}

float Host::GetFloatSettingValue(const char* section, const char* key, float default_value)
{
  return WebSettings::GetFloat(section, key, default_value);
}

double Host::GetDoubleSettingValue(const char* section, const char* key, double default_value)
{
  return WebSettings::GetDouble(section, key, default_value);
}

std::vector<std::string> Host::GetStringListSetting(const char* section, const char* key)
{
  return WebSettings::GetStringList(section, key);
}

// Base setting setters
void Host::SetBaseBoolSettingValue(const char* section, const char* key, bool value)
{
  WebSettings::SetBool(section, key, value);
}

void Host::SetBaseIntSettingValue(const char* section, const char* key, s32 value)
{
  WebSettings::SetInt(section, key, value);
}

void Host::SetBaseUIntSettingValue(const char* section, const char* key, u32 value)
{
  WebSettings::SetUInt(section, key, value);
}

void Host::SetBaseFloatSettingValue(const char* section, const char* key, float value)
{
  WebSettings::SetFloat(section, key, value);
}

void Host::SetBaseStringSettingValue(const char* section, const char* key, const char* value)
{
  WebSettings::SetString(section, key, value);
}

void Host::SetBaseStringListSettingValue(const char* section, const char* key, const std::vector<std::string>& values)
{
  WebSettings::SetStringList(section, key, values);
}

bool Host::AddValueToBaseStringListSetting(const char* section, const char* key, const char* value)
{
  return WebSettings::AddToStringList(section, key, value);
}

bool Host::RemoveValueFromBaseStringListSetting(const char* section, const char* key, const char* value)
{
  return WebSettings::RemoveFromStringList(section, key, value);
}

bool Host::ContainsBaseSettingValue(const char* section, const char* key)
{
  return WebSettings::ContainsValue(section, key);
}

void Host::DeleteBaseSettingValue(const char* section, const char* key)
{
  WebSettings::DeleteValue(section, key);
}

// Internal layer access
SettingsInterface* Host::Internal::GetBaseSettingsLayer()
{
  return WebSettings::GetInterface();
}

SettingsInterface* Host::Internal::GetGameSettingsLayer()
{
  return nullptr;  // No game-specific settings for web build
}

SettingsInterface* Host::Internal::GetInputSettingsLayer()
{
  return nullptr;  // No input profile settings for web build
}

void Host::Internal::SetBaseSettingsLayer(SettingsInterface* sif)
{
  // For web build, we directly use WebSettings::g_settings
  // This function is called from InitializeConfig, but we don't need to do anything
  // since we're already using WebSettings::g_settings directly
}

void Host::Internal::SetGameSettingsLayer(SettingsInterface* sif, std::unique_lock<std::mutex>& lock)
{
  // No game-specific settings for web build
}

void Host::Internal::SetInputSettingsLayer(SettingsInterface* sif, std::unique_lock<std::mutex>& lock)
{
  // No input profile settings for web build
}

std::string Host::GetHTTPUserAgent()
{
  return fmt::format("DuckStation for {} ({}) {}", TARGET_OS_STR, CPU_ARCH_STR, g_scm_tag_str);
}

#endif // __EMSCRIPTEN__

std::optional<WindowInfo> Host::AcquireRenderWindow(RenderAPI render_api, bool fullscreen, bool exclusive_fullscreen, Error* error)
{
  WindowInfo wi;
  wi.type = WindowInfo::Type::Surfaceless;  // WebGL canvas handled by Emscripten
  wi.surface_width = 640;
  wi.surface_height = 480;
  wi.surface_scale = 1.0f;
  wi.surface_refresh_rate = 60.0f;
  return wi;
}

void Host::ReleaseRenderWindow() {}
bool Host::IsFullscreen() { return false; }
void Host::SetFullscreen(bool enabled) {}
void Host::BeginTextInput() {}
void Host::EndTextInput() {}

bool Host::CreateAuxiliaryRenderWindow(s32 x, s32 y, u32 width, u32 height, std::string_view title,
                                       std::string_view icon_name, AuxiliaryRenderWindowUserData userdata,
                                       AuxiliaryRenderWindowHandle* handle, WindowInfo* wi, Error* error)
{
  return false;
}

void Host::DestroyAuxiliaryRenderWindow(AuxiliaryRenderWindowHandle handle, s32* pos_x, s32* pos_y, u32* width, u32* height) {}

void Host::OnSystemStarting() {}
void Host::OnSystemStarted() {}
void Host::OnSystemPaused() {}
void Host::OnSystemResumed() {}
void Host::OnSystemStopping() {}
void Host::OnSystemDestroyed() {}
void Host::OnSystemAbnormalShutdown(const std::string_view reason) {}
// void Host::OnGPUThreadRunIdleChanged(bool is_active) {}  // Removed from Host API
void Host::FrameDoneOnGPUThread(GPUBackend* gpu_backend, u32 frame_number) {}
void Host::OnPerformanceCountersUpdated(const GPUBackend* gpu_backend) {}

void Host::OnAchievementsLoginRequested(Achievements::LoginRequestReason reason) {}
void Host::OnAchievementsLoginSuccess(const char* username, u32 points, u32 sc_points, u32 unread_messages) {}
void Host::OnAchievementsRefreshed() {}
void Host::OnAchievementsActiveChanged(bool active) {}
void Host::OnAchievementsHardcoreModeChanged(bool enabled) {}
void Host::OnAchievementsAllProgressRefreshed() {}

void Host::SetMouseMode(bool relative, bool hide_cursor) {}
void Host::OnMediaCaptureStarted() {}
void Host::OnMediaCaptureStopped() {}
void Host::PumpMessagesOnCPUThread() {}

void Host::OnSystemGameChanged(const std::string& disc_path, const std::string& game_serial,
                               const std::string& game_name, GameHash game_hash) {}

void Host::OnSystemUndoStateAvailabilityChanged(bool available, u64 timestamp) {}

void Host::RunOnCPUThread(std::function<void()> function, bool block)
{
  function();  // Single-threaded, execute immediately
}

void Host::RunOnUIThread(std::function<void()> function, bool block)
{
  function();  // Single-threaded, execute immediately
}

void Host::RefreshGameListAsync(bool invalidate_cache)
{
  // Stub - game list not implemented for WebAssembly
}

void Host::CancelGameListRefresh()
{
  // Stub - game list not implemented for WebAssembly
}

void Host::OnGameListEntriesChanged(std::span<const u32> changed_indices)
{
  // Stub - game list not implemented for WebAssembly
}

void Host::OnGPUThreadRunIdleChanged(bool run_idle)
{
  // Stub - GPU thread not used in WebAssembly build
}

const char* Host::GetDefaultFullscreenUITheme()
{
  return "default";
}

// std::optional<WindowInfo> Host::GetTopLevelWindowInfo()  // Removed from Host API
// {
//   return Host::AcquireRenderWindow(RenderAPI::OpenGLES, false, false, nullptr);
// }

void Host::RequestResetSettings(bool system, bool controller)
{
  // Stub - settings reset not implemented for WebAssembly
}

void Host::RequestExitApplication(bool allow_confirm)
{
  // Stub - application exit handled by browser
}

void Host::RequestExitBigPicture()
{
  // Stub - Big Picture mode not used in WebAssembly
}

void Host::RequestSystemShutdown(bool allow_confirm, bool save_state, bool check_memcard_busy)
{
  if (System::IsValid())
    System::ShutdownSystem(save_state);
}

void Host::RequestResizeHostDisplay(s32 width, s32 height) {}
void Host::OpenURL(std::string_view url) {}

std::string Host::GetClipboardText() { return std::string(); }
bool Host::CopyTextToClipboard(std::string_view text) { return false; }

std::string Host::FormatNumber(NumberFormatType type, s64 value)
{
  return fmt::format("{}", value);
}

std::string Host::FormatNumber(NumberFormatType type, double value)
{
  return fmt::format("{}", value);
}

bool Host::ConfirmMessage(std::string_view title, std::string_view message)
{
  return false;
}

void Host::ConfirmMessageAsync(std::string_view title, std::string_view message, ConfirmMessageAsyncCallback callback,
                               std::string_view yes_text, std::string_view no_text)
{
  callback(false);
}

bool Host::ShouldPreferHostFileSelector()
{
  // Don't use native file selector in WebAssembly
  return false;
}

void Host::OpenHostFileSelectorAsync(std::string_view title, bool select_directory, FileSelectorCallback callback,
                                     FileSelectorFilters filters, std::string_view initial_directory)
{
  // File selection not implemented in WebAssembly
  // Browser file input should be used from JavaScript instead
  callback(std::string());
}

BEGIN_HOTKEY_LIST(g_host_hotkeys)
END_HOTKEY_LIST()

// ==============================================================================
// Emscripten Entry Point & Exported Functions
// ==============================================================================

#ifdef __EMSCRIPTEN__

extern "C" {

// Called from JavaScript to initialize the emulator
EMSCRIPTEN_KEEPALIVE
int duckstation_init()
{
  INFO_LOG("DuckStation Web - Initializing...");

  if (!WebHost::InitializeConfig())
  {
    ERROR_LOG("Failed to initialize config");
    return 0;
  }

  if (!WebHost::InitializeSystem())
  {
    ERROR_LOG("Failed to initialize system");
    return 0;
  }

  return 1;
}

// Called from JavaScript to load a BIOS file
EMSCRIPTEN_KEEPALIVE
int duckstation_load_bios(const char* path)
{
  INFO_LOG("Loading BIOS from: {}", path);
  WebHost::s_state.bios_path = path;

  // Extract directory and filename
  std::string path_str(path);
  size_t last_slash = path_str.find_last_of('/');
  std::string directory = (last_slash != std::string::npos) ? path_str.substr(0, last_slash) : "/bios";
  std::string filename = (last_slash != std::string::npos) ? path_str.substr(last_slash + 1) : path_str;

  INFO_LOG("BIOS directory: {}, filename: {}", directory, filename);

  // Update settings to use this BIOS
  auto lock = Host::GetSettingsLock();

  // Set both the search directory and the specific filename
  WebHost::s_state.settings_interface.SetStringValue("BIOS", "SearchDirectory", directory.c_str());
  WebHost::s_state.settings_interface.SetStringValue("BIOS", "Path", filename.c_str());

  Error error;
  if (!WebHost::s_state.settings_interface.Save(&error))
  {
    ERROR_LOG("Failed to save BIOS settings: {}", error.GetDescription());
  }

  // Also update EmuFolders::Bios to point to the correct directory
  EmuFolders::Bios = directory;
  INFO_LOG("Set EmuFolders::Bios to: {}", EmuFolders::Bios);

  return 1;
}

// Called from JavaScript to load and boot a game
EMSCRIPTEN_KEEPALIVE
int duckstation_boot_game(const char* path)
{
  INFO_LOG("=== duckstation_boot_game START ===");
  INFO_LOG("Booting game from: {}", path);

  INFO_LOG("Creating SystemBootParameters...");
  SystemBootParameters params;
  params.path = path;
  INFO_LOG("SystemBootParameters created successfully");

  INFO_LOG("Creating Error object...");
  Error error;
  INFO_LOG("Error object created");

  INFO_LOG("Calling System::BootSystem...");
  bool boot_result = System::BootSystem(std::move(params), &error);
  INFO_LOG("System::BootSystem returned: {}", boot_result);

  if (!boot_result)
  {
    ERROR_LOG("Failed to boot game: {}", error.GetDescription());
    return 0;
  }

  INFO_LOG("Setting running state...");
  WebHost::s_state.running = true;
  INFO_LOG("=== duckstation_boot_game SUCCESS ===");
  return 1;
}

// Main loop iteration (called by Emscripten's requestAnimationFrame)
EMSCRIPTEN_KEEPALIVE
void duckstation_frame()
{
  WebHost::MainLoopIteration();
}

// Shutdown
EMSCRIPTEN_KEEPALIVE
void duckstation_shutdown()
{
  INFO_LOG("Shutting down...");
  WebHost::s_state.running = false;
  WebHost::ShutdownSystem();
}

} // extern "C"

// Emscripten main loop callback
void emscripten_main_loop()
{
  duckstation_frame();
}

#endif // __EMSCRIPTEN__

// ==============================================================================
// Main Entry Point
// ==============================================================================

int main(int argc, char* argv[])
{
#ifdef __EMSCRIPTEN__
  INFO_LOG("DuckStation Web - Starting (initialize from JavaScript)");
  // Don't run the loop automatically; JavaScript will call duckstation_init() and start the loop
  return 0;
#else
  ERROR_LOG("This build is for web/Emscripten only");
  return 1;
#endif
}
