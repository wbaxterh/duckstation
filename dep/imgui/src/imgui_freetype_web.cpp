// SPDX-FileCopyrightText: 2025 DuckStation Web Contributors
// SPDX-License-Identifier: CC-BY-NC-ND-4.0

// WebAssembly stub for ImGui FreeType font loader
// FreeType is not used in WebAssembly builds, but this symbol is needed for linking

#include "imgui_freetype.h"

namespace ImGuiFreeType {

const ImFontLoader* GetFontLoader()
{
  // Return nullptr - FreeType not available in WebAssembly
  // imgui will fall back to the default STB TrueType loader
  return nullptr;
}

} // namespace ImGuiFreeType
