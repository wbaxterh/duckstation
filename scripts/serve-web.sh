#!/usr/bin/env bash
# ==============================================================================
# DuckStation Web - Development Server
# ==============================================================================
# Starts a local HTTP server with proper headers for SharedArrayBuffer support.
#
# COOP/COEP headers are required for:
#   - SharedArrayBuffer (needed for threading/atomics)
#   - High-resolution timers
#   - Other security-sensitive browser APIs
#
# Usage: bash scripts/serve-web.sh [port]
#   port: HTTP port (default: 8080)
# ==============================================================================

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
WEB_ROOT="${REPO_ROOT}/web-dist"
PORT="${1:-8080}"

echo "=========================================="
echo " DuckStation Web - Dev Server"
echo "=========================================="
echo ""
echo "Serving from: ${WEB_ROOT}"
echo "Port: ${PORT}"
echo ""

# Check if web-dist exists
if [ ! -d "${WEB_ROOT}" ]; then
  echo "ERROR: web-dist directory not found!"
  echo ""
  echo "Please build the project first:"
  echo "  bash scripts/build-web.sh"
  exit 1
fi

# Check if duckstation files exist
if [ ! -f "${WEB_ROOT}/duckstation/duckstation-web.js" ]; then
  echo "WARNING: duckstation-web.js not found in web-dist/duckstation/"
  echo ""
  echo "Please build the project first:"
  echo "  bash scripts/build-web.sh"
  echo ""
fi

# Try Python 3 first (most common)
if command -v python3 &> /dev/null; then
  echo "Using Python 3 HTTP server with COOP/COEP headers..."
  echo ""
  echo "Server starting on http://localhost:${PORT}"
  echo "Press Ctrl+C to stop"
  echo ""
  echo "=========================================="
  echo ""

  python3 - "${PORT}" "${WEB_ROOT}" << 'EOF'
#!/usr/bin/env python3
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
EOF

# Fallback to Node.js
elif command -v node &> /dev/null; then
  echo "Using Node.js HTTP server with COOP/COEP headers..."
  echo ""
  echo "Server starting on http://localhost:${PORT}"
  echo "Press Ctrl+C to stop"
  echo ""
  echo "=========================================="
  echo ""

  node - "${PORT}" "${WEB_ROOT}" << 'EOF'
const http = require('http');
const fs = require('fs');
const path = require('path');

const port = parseInt(process.argv[2]);
const webRoot = process.argv[3];

const mimeTypes = {
  '.html': 'text/html',
  '.js': 'application/javascript',
  '.wasm': 'application/wasm',
  '.css': 'text/css',
  '.json': 'application/json',
  '.png': 'image/png',
  '.jpg': 'image/jpeg',
  '.svg': 'image/svg+xml',
  '.ico': 'image/x-icon',
};

const server = http.createServer((req, res) => {
  let filePath = path.join(webRoot, req.url === '/' ? 'index.html' : req.url);

  fs.readFile(filePath, (err, content) => {
    if (err) {
      if (err.code === 'ENOENT') {
        res.writeHead(404);
        res.end('404 Not Found');
      } else {
        res.writeHead(500);
        res.end('500 Internal Server Error');
      }
    } else {
      const ext = path.extname(filePath);
      const contentType = mimeTypes[ext] || 'application/octet-stream';

      res.writeHead(200, {
        'Content-Type': contentType,
        'Cross-Origin-Opener-Policy': 'same-origin',
        'Cross-Origin-Embedder-Policy': 'require-corp',
        'Access-Control-Allow-Origin': '*',
        'Cache-Control': 'no-store, must-revalidate',
      });
      res.end(content);
    }
  });
});

server.listen(port, () => {
  console.log(`Server running at http://localhost:${port}/`);
});

process.on('SIGINT', () => {
  console.log('\nShutting down server...');
  process.exit(0);
});
EOF

else
  echo "ERROR: Neither Python 3 nor Node.js found!"
  echo ""
  echo "Please install one of the following:"
  echo "  - Python 3: apt install python3  (Debian/Ubuntu)"
  echo "              brew install python3 (macOS)"
  echo "  - Node.js:  apt install nodejs   (Debian/Ubuntu)"
  echo "              brew install node    (macOS)"
  exit 1
fi
