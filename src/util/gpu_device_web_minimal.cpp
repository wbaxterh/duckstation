// SPDX-FileCopyrightText: 2025 DuckStation Web Contributors
// SPDX-License-Identifier: CC-BY-NC-ND-4.0

// WebAssembly-specific GPU device stub implementations
// Only contains functions NOT already in gpu_device.cpp

#include "gpu_device.h"
#include "common/error.h"
#include "common/log.h"

LOG_CHANNEL(GPUDevice);

// Shader transpilation stub - not in base gpu_device.cpp
std::unique_ptr<GPUShader> GPUDevice::TranspileAndCreateShaderFromSource(
  GPUShaderStage stage, GPUShaderLanguage source_language, std::string_view source, const char* entry_point,
  GPUShaderLanguage target_language, u32 target_version, DynamicHeapArray<u8>* out_binary, Error* error)
{
  Error::SetStringView(error, "Shader transpilation not supported on WebAssembly");
  return {};
}
