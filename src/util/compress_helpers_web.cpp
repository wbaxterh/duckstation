// SPDX-FileCopyrightText: 2019-2025 Connor McLaughlin <stenzek@gmail.com>
// SPDX-License-Identifier: CC-BY-NC-ND-4.0

// WebAssembly stub implementation for compress_helpers
// Compression is not supported in web builds - only uncompressed data

#include "compress_helpers.h"

#include "common/error.h"
#include "common/file_system.h"
#include "common/log.h"
#include "common/path.h"

#include <zlib.h>  // Emscripten provides zlib via ports

LOG_CHANNEL(CompressHelpers);

namespace CompressHelpers {

std::optional<CompressType> GetCompressType(const std::string_view path, Error* error)
{
  // Web build only supports uncompressed
  return CompressType::Uncompressed;
}

const char* ZlibErrorToString(int res)
{
  switch (res)
  {
    case Z_OK: return "Z_OK";
    case Z_STREAM_END: return "Z_STREAM_END";
    case Z_NEED_DICT: return "Z_NEED_DICT";
    case Z_ERRNO: return "Z_ERRNO";
    case Z_STREAM_ERROR: return "Z_STREAM_ERROR";
    case Z_DATA_ERROR: return "Z_DATA_ERROR";
    case Z_MEM_ERROR: return "Z_MEM_ERROR";
    case Z_BUF_ERROR: return "Z_BUF_ERROR";
    case Z_VERSION_ERROR: return "Z_VERSION_ERROR";
    default: return "Z_UNKNOWN_ERROR";
  }
}

const char* SZErrorToString(int res)
{
  return "SZ_ERROR (not supported on WebAssembly)";
}

std::optional<size_t> GetDecompressedSize(CompressType type, std::span<const u8> data, Error* error)
{
  if (type == CompressType::Uncompressed)
    return data.size();

  Error::SetStringView(error, "Compression is not supported in WebAssembly builds");
  return std::nullopt;
}

std::optional<size_t> DecompressBuffer(std::span<u8> dst, CompressType type, std::span<const u8> data,
                                       std::optional<size_t> decompressed_size, Error* error)
{
  if (type == CompressType::Uncompressed)
  {
    if (dst.size() < data.size())
    {
      Error::SetStringFmt(error, "Destination buffer too small: {} < {}", dst.size(), data.size());
      return std::nullopt;
    }
    std::memcpy(dst.data(), data.data(), data.size());
    return data.size();
  }

  Error::SetStringView(error, "Compression is not supported in WebAssembly builds");
  return std::nullopt;
}

OptionalByteBuffer DecompressBuffer(CompressType type, std::span<const u8> data,
                                    std::optional<size_t> decompressed_size, Error* error)
{
  if (type == CompressType::Uncompressed)
  {
    ByteBuffer ret(data.size());
    std::memcpy(ret.data(), data.data(), data.size());
    return ret;
  }

  Error::SetStringView(error, "Compression is not supported in WebAssembly builds");
  return std::nullopt;
}

OptionalByteBuffer DecompressBuffer(CompressType type, OptionalByteBuffer data,
                                    std::optional<size_t> decompressed_size, Error* error)
{
  if (type == CompressType::Uncompressed && data.has_value())
    return data;

  Error::SetStringView(error, "Compression is not supported in WebAssembly builds");
  return std::nullopt;
}

bool DecompressBuffer(ByteBuffer& dst, CompressType type, std::span<const u8> data,
                      std::optional<size_t> decompressed_size, Error* error)
{
  if (type == CompressType::Uncompressed)
  {
    dst.resize(data.size());
    std::memcpy(dst.data(), data.data(), data.size());
    return true;
  }

  Error::SetStringView(error, "Compression is not supported in WebAssembly builds");
  return false;
}

OptionalByteBuffer DecompressFile(std::string_view path, std::span<const u8> data,
                                  std::optional<size_t> decompressed_size, Error* error)
{
  return DecompressBuffer(CompressType::Uncompressed, data, decompressed_size, error);
}

OptionalByteBuffer DecompressFile(std::string_view path, OptionalByteBuffer data,
                                  std::optional<size_t> decompressed_size, Error* error)
{
  return data;  // No compression in web builds
}

OptionalByteBuffer DecompressFile(const char* path, std::optional<size_t> decompressed_size, Error* error)
{
  return FileSystem::ReadBinaryFile(path, error);
}

OptionalByteBuffer DecompressFile(CompressType type, const char* path,
                                  std::optional<size_t> decompressed_size, Error* error)
{
  if (type != CompressType::Uncompressed)
  {
    Error::SetStringView(error, "Compression is not supported in WebAssembly builds");
    return std::nullopt;
  }
  return FileSystem::ReadBinaryFile(path, error);
}

OptionalByteBuffer CompressToBuffer(CompressType type, const void* data, size_t data_size, int clevel, Error* error)
{
  if (type != CompressType::Uncompressed)
  {
    Error::SetStringView(error, "Compression is not supported in WebAssembly builds");
    return std::nullopt;
  }

  ByteBuffer ret(data_size);
  std::memcpy(ret.data(), data, data_size);
  return ret;
}

OptionalByteBuffer CompressToBuffer(CompressType type, std::span<const u8> data, int clevel, Error* error)
{
  return CompressToBuffer(type, data.data(), data.size(), clevel, error);
}

OptionalByteBuffer CompressToBuffer(CompressType type, OptionalByteBuffer data, int clevel, Error* error)
{
  if (type != CompressType::Uncompressed || !data.has_value())
  {
    Error::SetStringView(error, "Compression is not supported in WebAssembly builds");
    return std::nullopt;
  }
  return data;
}

bool CompressToBuffer(ByteBuffer& dst, CompressType type, std::span<const u8> data, int clevel, Error* error)
{
  if (type != CompressType::Uncompressed)
  {
    Error::SetStringView(error, "Compression is not supported in WebAssembly builds");
    return false;
  }

  dst.resize(data.size());
  std::memcpy(dst.data(), data.data(), data.size());
  return true;
}

bool CompressToBuffer(ByteBuffer& dst, CompressType type, ByteBuffer data, int clevel, Error* error)
{
  if (type != CompressType::Uncompressed)
  {
    Error::SetStringView(error, "Compression is not supported in WebAssembly builds");
    return false;
  }

  dst = std::move(data);
  return true;
}

bool CompressToFile(const char* path, std::span<const u8> data, int clevel, bool atomic_write, Error* error)
{
  return CompressToFile(CompressType::Uncompressed, path, data, clevel, atomic_write, error);
}

bool CompressToFile(CompressType type, const char* path, std::span<const u8> data, int clevel,
                    bool atomic_write, Error* error)
{
  if (type != CompressType::Uncompressed)
  {
    Error::SetStringView(error, "Compression is not supported in WebAssembly builds");
    return false;
  }

  return atomic_write ? FileSystem::WriteAtomicRenamedFile(path, data.data(), data.size(), error) :
                        FileSystem::WriteBinaryFile(path, data.data(), data.size(), error);
}

} // namespace CompressHelpers
