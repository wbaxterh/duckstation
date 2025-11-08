// SPDX-FileCopyrightText: 2019-2024 Connor McLaughlin <stenzek@gmail.com>
// SPDX-License-Identifier: CC-BY-NC-ND-4.0

// WebAssembly-specific PIO implementation that bypasses virtual functions
// This avoids vtable issues in Emscripten builds

#include "pio.h"
#include "settings.h"

#include "util/state_wrapper.h"

#include "common/error.h"

namespace PIO {

// For web builds, we completely stub out PIO device support
// No virtual function calls, no polymorphism

bool Initialize(Error* error)
{
  // No PIO device for web build - return success
  return true;
}

void UpdateSettings(const Settings& old_settings)
{
  // No-op
}

void Shutdown()
{
  // No-op
}

void Reset()
{
  // No-op
}

bool DoState(StateWrapper& sw)
{
  // No-op - return success
  return !sw.HasError();
}

} // namespace PIO

// Stub the global PIO device pointer
std::unique_ptr<PIO::Device> g_pio_device;

// Stub the Device destructor
PIO::Device::~Device() = default;
