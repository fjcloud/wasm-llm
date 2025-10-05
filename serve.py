#!/usr/bin/env python3
from http.server import HTTPServer, SimpleHTTPRequestHandler

class Handler(SimpleHTTPRequestHandler):
    def end_headers(self):
        self.send_header('Cross-Origin-Opener-Policy', 'same-origin')
        self.send_header('Cross-Origin-Embedder-Policy', 'require-corp')
        self.send_header('Cache-Control', 'no-store')
        super().end_headers()

if __name__ == '__main__':
    print('🚀 Server running on http://localhost:8000')
    print('📡 COOP/COEP headers enabled for multi-threading')
    print('🧵 4-thread CPU support enabled (~4x faster)')
    print('🎮 WebGPU API enabled (experimental)')
    print('🤖 Qwen2.5-0.5B-Instruct ready to load')
    print('\nPress Ctrl+C to stop...\n')
    try:
        HTTPServer(('', 8000), Handler).serve_forever()
    except KeyboardInterrupt:
        print('\n👋 Server stopped')

