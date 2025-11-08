// SPDX-FileCopyrightText: 2025 DuckStation Web Contributors
// SPDX-License-Identifier: CC-BY-NC-ND-4.0

// WebAssembly stub for HTTP downloader
// Network requests should be made via JavaScript fetch() API instead

#include "http_downloader.h"
#include "common/error.h"
#include "common/log.h"

LOG_CHANNEL(HTTPDownloader);

std::unique_ptr<HTTPDownloader> HTTPDownloader::Create(std::string user_agent, Error* error)
{
  // HTTP downloading not supported in WebAssembly builds
  // Use JavaScript fetch() API from the web frontend instead
  Error::SetStringView(error, "HTTP downloading not supported in WebAssembly builds");
  return nullptr;
}
