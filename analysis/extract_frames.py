#!/usr/bin/env python3
"""Export complete replay frames as JSON for repeatable field analysis."""
import json,pathlib,re,subprocess,sys
ROOT=pathlib.Path(__file__).resolve().parents[1]
for arg in sys.argv[1:]:
 path=pathlib.Path(arg)
 output=subprocess.check_output([str(ROOT/'build/display_replay'),str(path),'--dump'],text=True)
 frames=[]
 for line in output.splitlines():
  if 's SPI-' not in line or '[partial' in line:continue
  parts=line.split(' | ')
  parse=lambda s:[int(v,16) for v in re.findall(r'0x([0-9A-F]{2})',s)]
  frames.append({'time':float(line.split('s ')[0]),'bus':1 if 's SPI-1' in line else 2,'mosi':parse(parts[0]),'miso':parse(parts[1]) if 's SPI-2' in line else []})
 dest=path.parent/'frames.json';dest.write_text(json.dumps(frames))
 print(str(dest),len(frames),'frames')
