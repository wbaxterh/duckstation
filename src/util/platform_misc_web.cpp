// SPDX-FileCopyrightText: 2025 DuckStation Web Contributors
// SPDX-License-Identifier: CC-BY-NC-ND-4.0

// WebAssembly stub implementations for platform-specific functions

#include "platform_misc.h"
#include "common/error.h"
#include "common/log.h"

LOG_CHANNEL(PlatformMisc);

namespace PlatformMisc {

bool InitializeSocketSupport(Error* error)
{
  // WebAssembly doesn't support raw sockets
  // WebSocket API should be used from JavaScript instead
  return true;
}

void SuspendScreensaver()
{
  // No-op on WebAssembly
}

void ResumeScreensaver()
{
  // No-op on WebAssembly
}

bool PlaySoundAsync(const char* path)
{
  // Sound playback not supported in WebAssembly builds
  // Web Audio API should be used from JavaScript instead
  return false;
}

bool SetWindowRoundedCornerState(void* window_handle, bool enabled, Error* error)
{
  // No window handles in WebAssembly
  return false;
}

} // namespace PlatformMisc

namespace Host {

std::optional<WindowInfo> GetTopLevelWindowInfo()
{
  // Return a basic window info for the canvas
  WindowInfo wi;
  wi.type = WindowInfo::Type::Surfaceless;
  wi.surface_width = 640;
  wi.surface_height = 480;
  wi.surface_scale = 1.0f;
  wi.surface_refresh_rate = 60.0f;
  return wi;
}

} // namespace Host
