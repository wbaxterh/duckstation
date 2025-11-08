# ==============================================================================
# DuckStation Web - Development Server (Windows PowerShell)
# ==============================================================================
# Starts a local HTTP server with proper headers for SharedArrayBuffer support.
#
# COOP/COEP headers are required for:
#   - SharedArrayBuffer (needed for threading/atomics)
#   - High-resolution timers
#   - Other security-sensitive browser APIs
#
# Usage: .\scripts\serve-web.ps1 [Port]
#   Port: HTTP port (default: 8080)
# ==============================================================================

param(
    [int]$Port = 8080
)

$ErrorActionPreference = "Stop"

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$RepoRoot = Split-Path -Parent $ScriptDir
$WebRoot = Join-Path $RepoRoot "web-dist"

Write-Host "=========================================="
Write-Host " DuckStation Web - Dev Server"
Write-Host "=========================================="
Write-Host ""
Write-Host "Serving from: $WebRoot"
Write-Host "Port: $Port"
Write-Host ""

# Check if web-dist exists
if (-Not (Test-Path $WebRoot)) {
    Write-Host "ERROR: web-dist directory not found!" -ForegroundColor Red
    Write-Host ""
    Write-Host "Please build the project first:"
    Write-Host "  .\scripts\build-web.ps1"
    exit 1
}

# Check if duckstation files exist
$jsFile = Join-Path $WebRoot "duckstation\duckstation-web.js"
if (-Not (Test-Path $jsFile)) {
    Write-Host "WARNING: duckstation-web.js not found in web-dist\duckstation\" -ForegroundColor Yellow
    Write-Host ""
    Write-Host "Please build the project first:"
    Write-Host "  .\scripts\build-web.ps1"
    Write-Host ""
}

# Try Python 3 first
$pythonCmd = Get-Command python3 -ErrorAction SilentlyContinue
if (-Not $pythonCmd) {
    $pythonCmd = Get-Command python -ErrorAction SilentlyContinue
}

if ($pythonCmd) {
    Write-Host "Using Python HTTP server with COOP/COEP headers..."
    Write-Host ""
    Write-Host "Server starting on http://localhost:$Port"
    Write-Host "Press Ctrl+C to stop"
    Write-Host ""
    Write-Host "=========================================="
    Write-Host ""

    $pythonScript = @"
import sys
import http.server
import socketserver
from functools import partial

class CORSRequestHandler(http.server.SimpleHTTPRequestHandler):
    def end_headers(self):
        # COOP/COEP headers for SharedArrayBuffer
        self.send_header('Cross-Origin-Opener-Policy', 'same-origin')
        self.send_header('Cross-Origin-Embedder-Policy', 'require-corp')
        # CORS headers for local development
        self.send_header('Access-Control-Allow-Origin', '*')
        self.send_header('Access-Control-Allow-Methods', 'GET, POST, OPTIONS')
        self.send_header('Access-Control-Allow-Headers', '*')
        # Cache control
        self.send_header('Cache-Control', 'no-store, must-revalidate')
        super().end_headers()

    def do_OPTIONS(self):
        self.send_response(200)
        self.end_headers()

if __name__ == '__main__':
    port = int(sys.argv[1])
    directory = sys.argv[2]

    handler = partial(CORSRequestHandler, directory=directory)

    with socketserver.TCPServer(("", port), handler) as httpd:
        print(f"Serving HTTP on 0.0.0.0 port {port} (http://localhost:{port}/) ...")
        try:
            httpd.serve_forever()
        except KeyboardInterrupt:
            print("\nShutting down server...")
            sys.exit(0)
"@

    # Save script to temp file
    $tempScript = Join-Path $env:TEMP "serve-duckstation.py"
    $pythonScript | Out-File -FilePath $tempScript -Encoding utf8

    # Run server
    Push-Location $WebRoot
    & $pythonCmd.Source $tempScript $Port $WebRoot
    Pop-Location

} else {
    # Fallback: PowerShell HTTP server (simpler, no COOP/COEP)
    Write-Host "Python not found. Using basic PowerShell HTTP server..." -ForegroundColor Yellow
    Write-Host ""
    Write-Host "WARNING: This server does NOT set COOP/COEP headers!" -ForegroundColor Yellow
    Write-Host "SharedArrayBuffer will not be available." -ForegroundColor Yellow
    Write-Host ""
    Write-Host "For full functionality, install Python 3:" -ForegroundColor Yellow
    Write-Host "  https://www.python.org/downloads/" -ForegroundColor Yellow
    Write-Host ""
    Write-Host "Server starting on http://localhost:$Port"
    Write-Host "Press Ctrl+C to stop"
    Write-Host ""
    Write-Host "=========================================="
    Write-Host ""

    Push-Location $WebRoot

    # Simple PowerShell HTTP server
    $listener = New-Object System.Net.HttpListener
    $listener.Prefixes.Add("http://localhost:$Port/")
    $listener.Start()

    Write-Host "Server running at http://localhost:$Port/"
    Write-Host ""

    try {
        while ($listener.IsListening) {
            $context = $listener.GetContext()
            $request = $context.Request
            $response = $context.Response

            $path = $request.Url.LocalPath
            if ($path -eq "/") { $path = "/index.html" }
            $filePath = Join-Path $WebRoot $path.TrimStart('/')

            if (Test-Path $filePath) {
                $content = [System.IO.File]::ReadAllBytes($filePath)
                $response.ContentType = switch ([System.IO.Path]::GetExtension($filePath)) {
                    ".html" { "text/html" }
                    ".js" { "application/javascript" }
                    ".wasm" { "application/wasm" }
                    ".css" { "text/css" }
                    ".json" { "application/json" }
                    ".png" { "image/png" }
                    ".jpg" { "image/jpeg" }
                    ".svg" { "image/svg+xml" }
                    default { "application/octet-stream" }
                }
                $response.ContentLength64 = $content.Length
                $response.OutputStream.Write($content, 0, $content.Length)
            } else {
                $response.StatusCode = 404
                $buffer = [System.Text.Encoding]::UTF8.GetBytes("404 Not Found")
                $response.ContentLength64 = $buffer.Length
                $response.OutputStream.Write($buffer, 0, $buffer.Length)
            }

            $response.Close()
        }
    } finally {
        $listener.Stop()
        Pop-Location
    }
}
