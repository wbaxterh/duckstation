// SPDX-FileCopyrightText: 2025 DuckStation Web Contributors
// SPDX-License-Identifier: CC-BY-NC-ND-4.0

// WebAssembly stub implementation for Image class
// Image loading/saving is not supported in WASM builds
// Browser-native APIs should be used via JavaScript interop instead

#include "image.h"
#include "common/error.h"
#include "common/log.h"

LOG_CHANNEL(Image);

Image::Image() = default;

Image::Image(u32 width, u32 height, ImageFormat format)
  : m_width(width), m_height(height), m_format(format)
{
  m_pitch = CalculatePitch(width, height, format);
  const u32 size = CalculateStorageSize(width, height, m_pitch, format);
  if (size > 0)
    m_pixels = Common::make_unique_aligned<u8[]>(VECTOR_ALIGNMENT, size);
}

Image::Image(u32 width, u32 height, ImageFormat format, const void* pixels, u32 pitch)
  : m_width(width), m_height(height), m_format(format), m_pitch(pitch)
{
  const u32 size = CalculateStorageSize(width, height, pitch, format);
  if (size > 0)
  {
    m_pixels = Common::make_unique_aligned<u8[]>(VECTOR_ALIGNMENT, size);
    std::memcpy(m_pixels.get(), pixels, size);
  }
}

Image::Image(u32 width, u32 height, ImageFormat format, PixelStorage pixels, u32 pitch)
  : m_width(width), m_height(height), m_format(format), m_pitch(pitch), m_pixels(std::move(pixels))
{
}

Image::Image(const Image& copy)
  : m_width(copy.m_width), m_height(copy.m_height), m_pitch(copy.m_pitch), m_format(copy.m_format)
{
  const u32 size = GetStorageSize();
  if (size > 0)
  {
    m_pixels = Common::make_unique_aligned<u8[]>(VECTOR_ALIGNMENT, size);
    std::memcpy(m_pixels.get(), copy.m_pixels.get(), size);
  }
}

Image::Image(Image&& move)
  : m_width(move.m_width), m_height(move.m_height), m_pitch(move.m_pitch), m_format(move.m_format),
    m_pixels(std::move(move.m_pixels))
{
  move.m_width = 0;
  move.m_height = 0;
  move.m_pitch = 0;
  move.m_format = ImageFormat::None;
}

Image& Image::operator=(const Image& copy)
{
  if (this != &copy)
  {
    m_width = copy.m_width;
    m_height = copy.m_height;
    m_pitch = copy.m_pitch;
    m_format = copy.m_format;

    const u32 size = GetStorageSize();
    if (size > 0)
    {
      m_pixels = Common::make_unique_aligned<u8[]>(VECTOR_ALIGNMENT, size);
      std::memcpy(m_pixels.get(), copy.m_pixels.get(), size);
    }
    else
    {
      m_pixels.reset();
    }
  }
  return *this;
}

Image& Image::operator=(Image&& move)
{
  if (this != &move)
  {
    m_width = move.m_width;
    m_height = move.m_height;
    m_pitch = move.m_pitch;
    m_format = move.m_format;
    m_pixels = std::move(move.m_pixels);

    move.m_width = 0;
    move.m_height = 0;
    move.m_pitch = 0;
    move.m_format = ImageFormat::None;
  }
  return *this;
}

const char* Image::GetFormatName(ImageFormat format)
{
  static constexpr const char* s_format_names[] = {
    "None", "RGBA8", "BGRA8", "RGB565", "RGB5A1", "A1BGR5", "BGR8", "BC1", "BC2", "BC3", "BC7",
  };
  return (static_cast<u32>(format) < static_cast<u32>(ImageFormat::MaxCount)) ? s_format_names[static_cast<u32>(format)] : "Unknown";
}

u32 Image::GetPixelSize(ImageFormat format)
{
  static constexpr u32 s_pixel_sizes[] = {
    0, // None
    4, // RGBA8
    4, // BGRA8
    2, // RGB565
    2, // RGB5A1
    2, // A1BGR5
    3, // BGR8
    0, // BC1 (compressed)
    0, // BC2 (compressed)
    0, // BC3 (compressed)
    0, // BC7 (compressed)
  };
  return (static_cast<u32>(format) < static_cast<u32>(ImageFormat::MaxCount)) ? s_pixel_sizes[static_cast<u32>(format)] : 0;
}

bool Image::IsCompressedFormat(ImageFormat format)
{
  return (format >= ImageFormat::BC1 && format <= ImageFormat::BC7);
}

u32 Image::CalculatePitch(u32 width, u32 height, ImageFormat format)
{
  if (IsCompressedFormat(format))
  {
    const u32 block_size = (format == ImageFormat::BC1) ? 8 : 16;
    const u32 blocks_wide = (width + 3) / 4;
    return blocks_wide * block_size;
  }
  else
  {
    return width * GetPixelSize(format);
  }
}

u32 Image::CalculateStorageSize(u32 width, u32 height, ImageFormat format)
{
  const u32 pitch = CalculatePitch(width, height, format);
  return CalculateStorageSize(width, height, pitch, format);
}

u32 Image::CalculateStorageSize(u32 width, u32 height, u32 pitch, ImageFormat format)
{
  if (IsCompressedFormat(format))
  {
    const u32 blocks_high = (height + 3) / 4;
    return pitch * blocks_high;
  }
  else
  {
    return pitch * height;
  }
}

u32 Image::GetBlocksWide() const
{
  return IsCompressedFormat(m_format) ? ((m_width + 3) / 4) : m_width;
}

u32 Image::GetBlocksHigh() const
{
  return IsCompressedFormat(m_format) ? ((m_height + 3) / 4) : m_height;
}

u32 Image::GetStorageSize() const
{
  return CalculateStorageSize(m_width, m_height, m_pitch, m_format);
}

std::span<const u8> Image::GetPixelsSpan() const
{
  return std::span<const u8>(m_pixels.get(), GetStorageSize());
}

std::span<u8> Image::GetPixelsSpan()
{
  return std::span<u8>(m_pixels.get(), GetStorageSize());
}

void Image::Clear()
{
  if (IsValid())
    std::memset(m_pixels.get(), 0, GetStorageSize());
}

void Image::Invalidate()
{
  m_width = 0;
  m_height = 0;
  m_pitch = 0;
  m_format = ImageFormat::None;
  m_pixels.reset();
}

void Image::Resize(u32 new_width, u32 new_height, bool preserve)
{
  Resize(new_width, new_height, m_format, preserve);
}

void Image::Resize(u32 new_width, u32 new_height, ImageFormat format, bool preserve)
{
  if (m_width == new_width && m_height == new_height && m_format == format)
    return;

  const u32 new_pitch = CalculatePitch(new_width, new_height, format);
  const u32 new_size = CalculateStorageSize(new_width, new_height, new_pitch, format);

  if (preserve && m_pixels && new_size > 0)
  {
    PixelStorage new_pixels = Common::make_unique_aligned<u8[]>(VECTOR_ALIGNMENT, new_size);

    const u32 copy_height = std::min(m_height, new_height);
    const u32 copy_pitch = std::min(m_pitch, new_pitch);

    for (u32 y = 0; y < copy_height; y++)
      std::memcpy(&new_pixels[y * new_pitch], &m_pixels[y * m_pitch], copy_pitch);

    m_pixels = std::move(new_pixels);
  }
  else if (new_size > 0)
  {
    m_pixels = Common::make_unique_aligned<u8[]>(VECTOR_ALIGNMENT, new_size);
  }
  else
  {
    m_pixels.reset();
  }

  m_width = new_width;
  m_height = new_height;
  m_pitch = new_pitch;
  m_format = format;
}

void Image::SetPixels(u32 width, u32 height, ImageFormat format, const void* pixels, u32 pitch)
{
  m_width = width;
  m_height = height;
  m_format = format;
  m_pitch = pitch;

  const u32 size = CalculateStorageSize(width, height, pitch, format);
  if (size > 0)
  {
    m_pixels = Common::make_unique_aligned<u8[]>(VECTOR_ALIGNMENT, size);
    std::memcpy(m_pixels.get(), pixels, size);
  }
  else
  {
    m_pixels.reset();
  }
}

void Image::SetPixels(u32 width, u32 height, ImageFormat format, PixelStorage pixels, u32 pitch)
{
  m_width = width;
  m_height = height;
  m_format = format;
  m_pitch = pitch;
  m_pixels = std::move(pixels);
}

bool Image::SetAllPixelsOpaque()
{
  ERROR_LOG("SetAllPixelsOpaque() not implemented for WebAssembly");
  return false;
}

Image::PixelStorage Image::TakePixels()
{
  m_width = 0;
  m_height = 0;
  m_pitch = 0;
  m_format = ImageFormat::None;
  return std::move(m_pixels);
}

bool Image::LoadFromFile(const char* filename, Error* error)
{
  Error::SetStringView(error, "Image loading not supported in WebAssembly builds");
  return false;
}

bool Image::LoadFromFile(std::string_view filename, std::FILE* fp, Error* error)
{
  Error::SetStringView(error, "Image loading not supported in WebAssembly builds");
  return false;
}

bool Image::LoadFromBuffer(std::string_view filename, std::span<const u8> data, Error* error)
{
  Error::SetStringView(error, "Image loading not supported in WebAssembly builds");
  return false;
}

bool Image::RasterizeSVG(const std::span<const u8> data, u32 width, u32 height, Error* error)
{
  Error::SetStringView(error, "SVG rasterization not supported in WebAssembly builds");
  return false;
}

bool Image::SaveToFile(const char* filename, u8 quality, Error* error) const
{
  Error::SetStringView(error, "Image saving not supported in WebAssembly builds");
  return false;
}

bool Image::SaveToFile(std::string_view filename, std::FILE* fp, u8 quality, Error* error) const
{
  Error::SetStringView(error, "Image saving not supported in WebAssembly builds");
  return false;
}

std::optional<DynamicHeapArray<u8>> Image::SaveToBuffer(std::string_view filename, u8 quality, Error* error) const
{
  Error::SetStringView(error, "Image saving not supported in WebAssembly builds");
  return std::nullopt;
}

std::optional<Image> Image::ConvertToRGBA8(Error* error) const
{
  if (!IsValid())
  {
    Error::SetStringView(error, "Invalid image");
    return std::nullopt;
  }

  if (m_format == ImageFormat::RGBA8)
    return Image(*this);

  Error::SetStringView(error, "Image format conversion not supported in WebAssembly builds");
  return std::nullopt;
}

bool Image::ConvertToRGBA8(void* RESTRICT pixels_out, u32 pixels_out_pitch, const void* RESTRICT pixels_in,
                           u32 pixels_in_pitch, u32 width, u32 height, ImageFormat format, Error* error)
{
  Error::SetStringView(error, "Image format conversion not supported in WebAssembly builds");
  return false;
}

void Image::FlipY()
{
  if (!IsValid())
    return;

  const u32 row_size = IsCompressedFormat(m_format) ? m_pitch : (m_width * GetPixelSize(m_format));
  PixelStorage temp_row = Common::make_unique_aligned<u8[]>(VECTOR_ALIGNMENT, row_size);

  for (u32 y = 0; y < m_height / 2; y++)
  {
    u8* top = &m_pixels[y * m_pitch];
    u8* bottom = &m_pixels[(m_height - y - 1) * m_pitch];

    std::memcpy(temp_row.get(), top, row_size);
    std::memcpy(top, bottom, row_size);
    std::memcpy(bottom, temp_row.get(), row_size);
  }
}
