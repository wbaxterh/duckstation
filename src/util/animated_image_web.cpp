// SPDX-FileCopyrightText: 2025 DuckStation Web Contributors
// SPDX-License-Identifier: CC-BY-NC-ND-4.0

// WebAssembly stub for animated image (GIF) support

#include "animated_image.h"
#include "common/error.h"
#include "common/log.h"

LOG_CHANNEL(Image);

AnimatedImage::AnimatedImage(u32 width, u32 height, u32 num_frames, const FrameDelay& frame_delay)
{
  // Stub - animated images not supported in WebAssembly
}

void AnimatedImage::SetPixels(u32 frame, const void* pixels, u32 pitch)
{
  // Stub - no-op
}

bool AnimatedImage::SaveToFile(const char* path, u8 quality, Error* error) const
{
  Error::SetStringView(error, "Animated image saving not supported in WebAssembly builds");
  return false;
}
