// SPDX-FileCopyrightText: 2025 DuckStation Web Contributors
// SPDX-License-Identifier: CC-BY-NC-ND-4.0

// WebAssembly stub for audio stream

#include "audio_stream.h"
#include "common/error.h"
#include "common/log.h"

LOG_CHANNEL(AudioStream);

// Static functions for backend/stretch mode names
std::optional<AudioBackend> AudioStream::ParseBackendName(const char* str)
{
  return std::nullopt;
}

const char* AudioStream::GetBackendName(AudioBackend backend)
{
  switch (backend)
  {
    case AudioBackend::Null:
      return "Null";
    default:
      return "Unknown";
  }
}

const char* AudioStream::GetBackendDisplayName(AudioBackend backend)
{
  switch (backend)
  {
    case AudioBackend::Null:
      return "Null (No Output)";
    default:
      return "Unknown";
  }
}

const char* AudioStream::GetStretchModeName(AudioStretchMode mode)
{
  switch (mode)
  {
    case AudioStretchMode::Off:
      return "Off";
    case AudioStretchMode::Resample:
      return "Resample";
    case AudioStretchMode::TimeStretch:
      return "TimeStretch";
    default:
      return "Unknown";
  }
}

const char* AudioStream::GetStretchModeDisplayName(AudioStretchMode mode)
{
  switch (mode)
  {
    case AudioStretchMode::Off:
      return "Disabled";
    case AudioStretchMode::Resample:
      return "Resampling";
    case AudioStretchMode::TimeStretch:
      return "Time Stretching";
    default:
      return "Unknown";
  }
}

std::optional<AudioStretchMode> AudioStream::ParseStretchMode(const char* name)
{
  if (std::strcmp(name, "Off") == 0)
    return AudioStretchMode::Off;
  else if (std::strcmp(name, "Resample") == 0)
    return AudioStretchMode::Resample;
  else if (std::strcmp(name, "TimeStretch") == 0)
    return AudioStretchMode::TimeStretch;

  return std::nullopt;
}

// AudioStreamParameters methods
void AudioStreamParameters::Load(const SettingsInterface& si, const char* section)
{
  // Stub - no settings persistence in WebAssembly
}

void AudioStreamParameters::Save(SettingsInterface& si, const char* section) const
{
  // Stub - no settings persistence in WebAssembly
}

bool AudioStreamParameters::operator==(const AudioStreamParameters& rhs) const
{
  return stretch_mode == rhs.stretch_mode &&
         output_latency_minimal == rhs.output_latency_minimal &&
         output_latency_ms == rhs.output_latency_ms &&
         buffer_ms == rhs.buffer_ms &&
         stretch_sequence_length_ms == rhs.stretch_sequence_length_ms &&
         stretch_seekwindow_ms == rhs.stretch_seekwindow_ms &&
         stretch_overlap_ms == rhs.stretch_overlap_ms &&
         stretch_use_quickseek == rhs.stretch_use_quickseek &&
         stretch_use_aa_filter == rhs.stretch_use_aa_filter;
}

bool AudioStreamParameters::operator!=(const AudioStreamParameters& rhs) const
{
  return !(*this == rhs);
}

// AudioStream class methods
std::unique_ptr<AudioStream> AudioStream::CreateStream(AudioBackend backend, u32 sample_rate,
                                                       const AudioStreamParameters& parameters, const char* driver_name,
                                                       const char* device_name, Error* error)
{
  // Stub - audio output not implemented for WebAssembly
  // Web Audio API should be used from JavaScript instead
  return nullptr;
}

std::unique_ptr<AudioStream> AudioStream::CreateNullStream(u32 sample_rate, u32 buffer_ms)
{
  // Stub - null stream not needed in WebAssembly
  return nullptr;
}

void AudioStream::SetOutputVolume(u32 volume)
{
  // Stub - volume control not implemented
  m_volume = volume;
}

void AudioStream::SetNominalRate(float tempo)
{
  // Stub - tempo control not implemented
  m_nominal_rate = tempo;
}

void AudioStream::BeginWrite(SampleType** buffer_ptr, u32* num_frames)
{
  // Stub - no audio buffer in WebAssembly
  *buffer_ptr = nullptr;
  *num_frames = 0;
}

void AudioStream::EndWrite(u32 num_frames)
{
  // Stub - no audio buffer in WebAssembly
}

void AudioStream::SetStretchMode(AudioStretchMode mode)
{
  // Stub - stretch mode not implemented
  m_parameters.stretch_mode = mode;
}

void AudioStream::EmptyStretchBuffers()
{
  // Stub - no stretch buffers in WebAssembly
}

u32 AudioStream::GetBufferedFramesRelaxed() const
{
  // Stub - no buffering in WebAssembly
  return 0;
}

u32 AudioStream::GetMSForBufferSize(u32 sample_rate, u32 buffer_size)
{
  // Calculate milliseconds for given buffer size
  return (buffer_size * 1000) / sample_rate;
}
