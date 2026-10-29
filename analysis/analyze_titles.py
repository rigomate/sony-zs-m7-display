#!/usr/bin/env python3
"""Reproduce title evidence from raw captures via the existing frame reader."""
import collections
import glob
import pathlib
import re
import subprocess

ROOT = pathlib.Path(__file__).resolve().parents[1]
EXPECTED = ['0123456789AB', 'CDEFGHIJKLMN', 'OPQRSTUVWXYZ', 'abcdefghijkl',
            'mnopqrstuvwx', 'yz!?+-_./:;', 'b', 'H', 'PLAY', 'PLAY', 'TRACK 10', '         LON']

def decode(data):
    return ''.join(chr(v) for v in data)

for path, expected in zip(sorted(glob.glob(str(ROOT / 'captures/title-test-*-raw/digital.csv'))), EXPECTED):
    output = subprocess.check_output([str(ROOT / 'build/display_replay'), path, '--dump'], text=True)
    streams = collections.defaultdict(list)
    for line in output.splitlines():
        if 's SPI-' not in line or '[partial' in line:
            continue
        parts = line.split(' | ')
        bus = 'SDT1' if 's SPI-1' in line else 'SDI2'
        streams[bus].append([int(v, 16) for v in re.findall(r'0x([0-9A-F]{2})', parts[0])])
        if bus == 'SDI2':
            streams['SDO2'].append([int(v, 16) for v in re.findall(r'0x([0-9A-F]{2})', parts[1])])
    counts = collections.Counter(decode(f[2:14]) for f in streams['SDI2'])
    # Same transform, every possible byte-aligned start; not an exhaustive
    # search of arbitrary packing, encodings or alternate clock modes.
    matches = {bus: sum(expected in decode(f) for f in frames) for bus, frames in streams.items()}
    print(pathlib.Path(path).parent.name)
    print('  last window:', repr(decode(streams['SDI2'][-1][2:14])))
    print('  common windows:', counts.most_common(3))
    print('  expected substring matches by bus:', matches)
