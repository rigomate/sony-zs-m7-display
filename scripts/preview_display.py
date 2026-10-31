#!/usr/bin/env python3
"""Local dashboard preview using an explicit JSON fixture; never accesses hardware."""
import argparse
from http.server import BaseHTTPRequestHandler, HTTPServer
import json
from pathlib import Path

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--fixture', type=Path, required=True)
parser.add_argument('--port', type=int, default=8088)
args = parser.parse_args()
page = (Path(__file__).resolve().parents[1] / 'firmware/esp32/web/index.html').read_bytes()
tft_page = (Path(__file__).resolve().parents[1] / 'firmware/esp32/web/tft.html').read_bytes()
fixture = args.fixture.read_bytes()
json.loads(fixture)

class Handler(BaseHTTPRequestHandler):
    def do_GET(self):
        path = self.path.partition('?')[0]
        if path == '/':
            content, mime = page, 'text/html; charset=utf-8'
        elif path in ('/tft', '/tft/'):
            content, mime = tft_page, 'text/html; charset=utf-8'
        elif path == '/api/state':
            content, mime = fixture, 'application/json'
        else:
            self.send_error(404)
            return
        self.send_response(200)
        self.send_header('Content-Type', mime)
        self.send_header('Cache-Control', 'no-store')
        self.send_header('Content-Length', str(len(content)))
        self.end_headers()
        self.wfile.write(content)

print(f'Fixture preview: http://127.0.0.1:{args.port}/', flush=True)
HTTPServer(('127.0.0.1', args.port), Handler).serve_forever()
