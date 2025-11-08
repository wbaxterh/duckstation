// SPDX-FileCopyrightText: 2025 DuckStation Web Contributors
// SPDX-License-Identifier: CC-BY-NC-ND-4.0

// WebAssembly stub implementations for CPU recompiler functions
// These are not used since JIT is disabled, but need to link

#include "cpu_core_private.h"
#include "cpu_code_cache_private.h"
#include "common/assert.h"
#include "common/log.h"

LOG_CHANNEL(CPU);

namespace CPU {

// Recompiler stubs - should never be called on WebAssembly
u32 CodeCache::EmitASMFunctions(void* code, u32 code_size)
{
  Panic("JIT recompiler not supported on WebAssembly");
  return 0;
}

u32 CodeCache::EmitJump(void* code, const void* dst, bool flush_icache)
{
  Panic("JIT recompiler not supported on WebAssembly");
  return 0;
}

void CheckAndUpdateICacheTags(u32 line_count)
{
  Panic("JIT recompiler not supported on WebAssembly");
}

TickCount GetInstructionReadTicks(u32 pc)
{
  // Stub implementation - return 0 ticks
  return 0;
}

bool SafeReadInstruction(VirtualMemoryAddress addr, u32* value)
{
  // Stub - not used without JIT
  *value = 0;
  return false;
}

bool SafeWriteMemoryByte(VirtualMemoryAddress addr, u8 value)
{
  // Stub - not used without JIT
  return false;
}

bool SafeWriteMemoryHalfWord(VirtualMemoryAddress addr, u16 value)
{
  // Stub - not used without JIT
  return false;
}

bool SafeWriteMemoryWord(VirtualMemoryAddress addr, u32 value)
{
  // Stub - not used without JIT
  return false;
}

bool SafeReadMemoryByte(VirtualMemoryAddress addr, u8* value)
{
  // Stub - not used without JIT
  *value = 0;
  return false;
}

bool SafeReadMemoryHalfWord(VirtualMemoryAddress addr, u16* value)
{
  // Stub - not used without JIT
  *value = 0;
  return false;
}

bool SafeReadMemoryWord(VirtualMemoryAddress addr, u32* value)
{
  // Stub - not used without JIT
  *value = 0;
  return false;
}

bool SafeReadMemoryCString(VirtualMemoryAddress addr, SmallStringBase* value, u32 max_length)
{
  // Stub - safe read not implemented
  return false;
}

bool SafeReadMemoryBytes(VirtualMemoryAddress addr, void* data, u32 length)
{
  // Stub - safe read not implemented
  return false;
}

bool SafeWriteMemoryBytes(VirtualMemoryAddress addr, const void* data, u32 length)
{
  // Stub - safe write not implemented
  return false;
}

bool SafeWriteMemoryBytes(VirtualMemoryAddress addr, const std::span<const u8> data)
{
  // Stub - safe write not implemented
  return false;
}

bool SafeZeroMemoryBytes(VirtualMemoryAddress addr, u32 length)
{
  // Stub - safe write not implemented
  return false;
}

void ClearICache()
{
  // Stub - I-cache not used in WebAssembly interpreter
}

void UpdateMemoryPointers()
{
  // Stub - memory pointers not used in WebAssembly build
}

} // namespace CPU

// Forward declarations for the out-of-line implementations in cpu_core.cpp
namespace CPU {
bool FetchInstruction();
bool FetchInstructionForInterpreterFallback();
bool ReadMemoryByte(VirtualMemoryAddress addr, u8* value);
bool ReadMemoryHalfWord(VirtualMemoryAddress addr, u16* value);
bool ReadMemoryWord(VirtualMemoryAddress addr, u32* value);
bool WriteMemoryByte(VirtualMemoryAddress addr, u32 value);
bool WriteMemoryHalfWord(VirtualMemoryAddress addr, u32 value);
bool WriteMemoryWord(VirtualMemoryAddress addr, u32 value);
}
