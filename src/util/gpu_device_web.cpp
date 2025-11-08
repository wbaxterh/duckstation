// SPDX-FileCopyrightText: 2019-2024 Connor McLaughlin <stenzek@gmail.com>
// SPDX-License-Identifier: CC-BY-NC-ND-4.0

// WebAssembly-specific GPU device implementation
// Uses OpenGL ES via WebGL, without dynamic shader compilation

#include "gpu_device.h"
#include "opengl_device.h"

#include "common/assert.h"
#include "common/error.h"
#include "common/log.h"

LOG_CHANNEL(GPUDevice);

// Global GPU device pointer
std::unique_ptr<GPUDevice> g_gpu_device;

// Statistics
size_t GPUDevice::s_total_vram_usage = 0;
GPUDevice::Statistics GPUDevice::s_stats = {};

// Sampler implementations
GPUSampler::GPUSampler() = default;
GPUSampler::~GPUSampler() = default;

GPUSampler::Config GPUSampler::GetNearestConfig()
{
  Config config = {};
  config.address_u = GPUSampler::AddressMode::ClampToEdge;
  config.address_v = GPUSampler::AddressMode::ClampToEdge;
  config.address_w = GPUSampler::AddressMode::ClampToEdge;
  config.min_filter = GPUSampler::Filter::Nearest;
  config.mag_filter = GPUSampler::Filter::Nearest;
  return config;
}

GPUSampler::Config GPUSampler::GetLinearConfig()
{
  Config config = {};
  config.address_u = GPUSampler::AddressMode::ClampToEdge;
  config.address_v = GPUSampler::AddressMode::ClampToEdge;
  config.address_w = GPUSampler::AddressMode::ClampToEdge;
  config.min_filter = GPUSampler::Filter::Linear;
  config.mag_filter = GPUSampler::Filter::Linear;
  return config;
}

// Shader implementations
GPUShader::GPUShader(GPUShaderStage stage) : m_stage(stage)
{
}

GPUShader::~GPUShader() = default;

const char* GPUShader::GetStageName(GPUShaderStage stage)
{
  static constexpr std::array<const char*, static_cast<u32>(GPUShaderStage::MaxCount)> names = {
    "Vertex", "Fragment", "Geometry", "Compute"
  };
  return names[static_cast<u32>(stage)];
}

// Pipeline implementations
GPUPipeline::GPUPipeline() = default;
GPUPipeline::~GPUPipeline() = default;

GPUPipeline::BlendState GPUPipeline::BlendState::GetNoBlendingState()
{
  BlendState bs = {};
  bs.enable = false;
  return bs;
}

GPUPipeline::RasterizationState GPUPipeline::RasterizationState::GetNoCullState()
{
  RasterizationState rs = {};
  rs.cull_mode = GPUPipeline::CullMode::None;
  return rs;
}

GPUPipeline::DepthState GPUPipeline::DepthState::GetNoTestsState()
{
  DepthState ds = {};
  ds.depth_test = GPUPipeline::DepthFunc::Always;
  ds.depth_write = false;
  return ds;
}

// SwapChain implementations
GPUSwapChain::GPUSwapChain(const WindowInfo& wi, GPUVSyncMode vsync_mode, bool allow_present_throttle)
  : m_window_info(wi), m_vsync_mode(vsync_mode), m_allow_present_throttle(allow_present_throttle)
{
}

GPUSwapChain::~GPUSwapChain() = default;

// Device implementations
GPUDevice::GPUDevice()
{
  ResetStatistics();
}

GPUDevice::~GPUDevice() = default;

RenderAPI GPUDevice::GetPreferredAPI()
{
  // WebAssembly always uses OpenGL ES via WebGL
  return RenderAPI::OpenGLES;
}

const char* GPUDevice::RenderAPIToString(RenderAPI api)
{
  switch (api)
  {
    case RenderAPI::OpenGLES: return "OpenGLES";
    case RenderAPI::OpenGL: return "OpenGL";
    default: return "Unknown";
  }
}

const char* GPUDevice::ShaderLanguageToString(GPUShaderLanguage language)
{
  switch (language)
  {
    case GPUShaderLanguage::GLSL: return "GLSL";
    case GPUShaderLanguage::GLSLES: return "GLSLES";
    default: return "Unknown";
  }
}

const char* GPUDevice::VSyncModeToString(GPUVSyncMode mode)
{
  static constexpr std::array<const char*, static_cast<size_t>(GPUVSyncMode::Count)> vsync_modes = {{
    "Disabled",
    "FIFO",
    "Mailbox",
  }};
  return vsync_modes[static_cast<size_t>(mode)];
}

bool GPUDevice::IsSameRenderAPI(RenderAPI lhs, RenderAPI rhs)
{
  return (lhs == rhs || ((lhs == RenderAPI::OpenGL || lhs == RenderAPI::OpenGLES) &&
                         (rhs == RenderAPI::OpenGL || rhs == RenderAPI::OpenGLES)));
}

GPUDevice::AdapterInfoList GPUDevice::GetAdapterListForAPI(RenderAPI api)
{
  // WebGL doesn't support adapter enumeration
  return AdapterInfoList();
}

std::unique_ptr<GPUDevice> GPUDevice::CreateDeviceForAPI(RenderAPI api)
{
  // WebAssembly only supports OpenGL ES
  if (api == RenderAPI::OpenGL || api == RenderAPI::OpenGLES)
    return std::make_unique<OpenGLDevice>();

  return {};
}

void GPUDevice::ResetStatistics()
{
  s_stats = {};
}

GPUDriverType GPUDevice::GuessDriverType(u32 pci_vendor_id, std::string_view vendor_name, std::string_view adapter_name)
{
  // WebAssembly doesn't have PCI vendor IDs
  return GPUDriverType::Unknown;
}

void GPUDevice::SetDriverType(GPUDriverType type)
{
  m_driver_type = type;
  INFO_LOG("Driver type set to WebGL.");
}

// Shader compilation stubs - WebGL shaders are compiled directly by OpenGL backend
std::optional<DynamicHeapArray<u8>> GPUDevice::OptimizeVulkanSpv(const std::span<const u8> spirv, Error* error)
{
  Error::SetStringView(error, "SPIR-V not supported on WebAssembly");
  return std::nullopt;
}

bool GPUDevice::CompileGLSLShaderToVulkanSpv(GPUShaderStage stage, GPUShaderLanguage source_language,
                                             std::string_view source, const char* entry_point, bool optimization,
                                             bool nonsemantic_debug_info, DynamicHeapArray<u8>* out_binary,
                                             Error* error)
{
  Error::SetStringView(error, "SPIR-V compilation not supported on WebAssembly");
  return false;
}

bool GPUDevice::TranslateVulkanSpvToLanguage(const std::span<const u8> spirv, GPUShaderStage stage,
                                             GPUShaderLanguage target_language, u32 target_version, std::string* output,
                                             Error* error)
{
  Error::SetStringView(error, "SPIR-V translation not supported on WebAssembly");
  return false;
}

std::unique_ptr<GPUShader> GPUDevice::TranspileAndCreateShaderFromSource(
  GPUShaderStage stage, GPUShaderLanguage source_language, std::string_view source, const char* entry_point,
  GPUShaderLanguage target_language, u32 target_version, DynamicHeapArray<u8>* out_binary, Error* error)
{
  Error::SetStringView(error, "Shader transpilation not supported on WebAssembly");
  return {};
}

// Texture pool stubs - not needed for WebAssembly
void GPUDevice::RecycleTexture(std::unique_ptr<GPUTexture> texture)
{
  // Just discard the texture - no pooling in WebAssembly
}

void GPUDevice::PurgeTexturePool()
{
  // No-op - no pool to purge
}

bool GPUDevice::UsesLowerLeftOrigin() const
{
  // OpenGL/WebGL uses lower-left origin
  return true;
}

std::unique_ptr<GPUTexture> GPUDevice::FetchTexture(u32 width, u32 height, u32 layers, u32 levels, u32 samples,
                                                     GPUTexture::Type type, GPUTexture::Format format,
                                                     GPUTexture::Flags flags, const void* data, u32 data_stride,
                                                     Error* error)
{
  // Stub - texture pooling not implemented for WebAssembly
  Error::SetStringView(error, "FetchTexture not implemented for WebAssembly");
  return nullptr;
}

std::unique_ptr<GPUTexture> GPUDevice::FetchAndUploadTextureImage(const Image& image, GPUTexture::Flags flags,
                                                                   Error* error)
{
  // Stub - texture upload from Image not implemented for WebAssembly
  Error::SetStringView(error, "FetchAndUploadTextureImage not implemented for WebAssembly");
  return nullptr;
}

std::unique_ptr<GPUShader> GPUDevice::CreateShader(GPUShaderStage stage, GPUShaderLanguage language,
                                                    std::string_view source, Error* error, const char* entry_point)
{
  // This should be implemented by the OpenGL device
  Error::SetStringView(error, "CreateShader should be called on OpenGL device, not base GPUDevice");
  return nullptr;
}

void GPUDevice::SetRenderTarget(GPUTexture* rt, GPUTexture* ds, GPUPipeline::RenderPassFlag flags)
{
  // Stub - should be implemented by OpenGL device
}

void GPUDevice::SetViewportAndScissor(s32 x, s32 y, s32 width, s32 height)
{
  // Stub - should be implemented by OpenGL device
}

bool GPUDevice::ResizeTexture(std::unique_ptr<GPUTexture>* tex, u32 new_width, u32 new_height, GPUTexture::Type type,
                               GPUTexture::Format format, GPUTexture::Flags flags, bool preserve, Error* error)
{
  // Stub - texture resizing not implemented
  Error::SetStringView(error, "ResizeTexture not implemented for WebAssembly");
  return false;
}

GSVector4i GPUDevice::FlipToLowerLeft(GSVector4i rc, s32 target_height)
{
  // Flip Y coordinate for lower-left origin (OpenGL)
  return GSVector4i(rc.x, target_height - rc.w, rc.z, target_height - rc.y);
}

// SwapChain methods
GSVector4i GPUSwapChain::PreRotateClipRect(WindowInfo::PreRotation rotation, GSVector2i size, const GSVector4i& v)
{
  // No pre-rotation needed for WebGL
  return v;
}

// BlendState methods
GPUPipeline::BlendState GPUPipeline::BlendState::GetAlphaBlendingState()
{
  BlendState bs = {};
  bs.enable = true;
  bs.src_blend = GPUPipeline::BlendFunc::SrcAlpha;
  bs.dst_blend = GPUPipeline::BlendFunc::InvSrcAlpha;
  bs.blend_op = GPUPipeline::BlendOp::Add;
  return bs;
}
