// SPDX-FileCopyrightText: 2019-2024 Connor McLaughlin <stenzek@gmail.com>
// SPDX-License-Identifier: CC-BY-NC-ND-4.0

#include "web_settings.h"

namespace WebSettings {

// Global settings instance - single INISettingsInterface (no layers)
// This avoids virtual function calls through LayeredSettingsInterface
INISettingsInterface g_settings;
std::mutex g_settings_mutex;

} // namespace WebSettings
