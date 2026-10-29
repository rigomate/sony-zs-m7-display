#!/usr/bin/env python3
"""Render dashboard states with local Chromium and a capture-derived API fixture."""
import argparse
import copy
from http.server import BaseHTTPRequestHandler, HTTPServer
import json
from pathlib import Path
import subprocess
import tempfile
import threading

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--fixture', type=Path, required=True)
parser.add_argument('--browser', default='chromium')
args = parser.parse_args()
base = json.loads(args.fixture.read_text())
page = (Path(__file__).resolve().parents[1] / 'firmware/esp32/web/index.html').read_bytes()
active = base
offline = False

class Handler(BaseHTTPRequestHandler):
    def log_message(self, *args):
        pass
    def do_GET(self):
        if self.path == '/':
            data, mime = page, 'text/html; charset=utf-8'
        elif self.path == '/api/state':
            if offline:
                self.send_error(503)
                return
            data, mime = json.dumps(active).encode(), 'application/json'
        else:
            self.send_error(404)
            return
        self.send_response(200)
        self.send_header('Content-Type', mime)
        self.send_header('Content-Length', str(len(data)))
        self.end_headers()
        self.wfile.write(data)

server = HTTPServer(('127.0.0.1', 0), Handler)
threading.Thread(target=server.serve_forever, daemon=True).start()

def render(name, expected, absent=()):
    with tempfile.TemporaryDirectory(prefix='zs-dashboard-') as profile:
        result = subprocess.run([args.browser, '--headless', '--no-sandbox', '--disable-gpu',
            '--disable-dev-shm-usage', '--no-first-run', '--no-default-browser-check',
            '--user-data-dir='+profile, '--virtual-time-budget=3000', '--dump-dom',
            f'http://127.0.0.1:{server.server_port}/'], capture_output=True, text=True,
            check=True, timeout=25)
        # Only test rendered markup; strings in JS source are not visible UI.
        dom = result.stdout.split('<script>')[0]
        for text in expected:
            assert text in dom, f'{name}: missing {text}'
        for text in absent:
            assert text not in dom, f'{name}: unexpected {text}'
        print('PASS dashboard:', name, flush=True)

try:
    render('live capture', ['class="status live"', 'id="track">20', 'id="time">0:14',
                           'class="track on current"'])
    active = copy.deepcopy(base)
    for b in active['buses']:
        b['fresh'] = False
        b['age_ms'] = 2000
    render('stale buses', ['Waiting for signals', 'class="numbers stale"',
                          'Last received title · signal stale'], ['class="status live"'])
    active = copy.deepcopy(base)
    active['title'] = {'text': None, 'codes': [1]+[255]*11}
    render('unknown title control', ['Display transition · unknown cells shown as dots'])
    offline = True
    render('device disconnected', ['Device disconnected', 'class="numbers stale"'],
                                 ['class="status live"'])
finally:
    server.shutdown()
