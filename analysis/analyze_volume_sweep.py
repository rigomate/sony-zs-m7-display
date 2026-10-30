#!/usr/bin/env python3
"""Analyse extracted complete frames from the user-labelled volume sweep."""
import collections,csv,json,pathlib,sys
root=pathlib.Path(__file__).resolve().parents[1]
p=pathlib.Path(sys.argv[1]) if len(sys.argv)>1 else root/'captures/manual/volume-from-min-to-max-raw/frames.json'
frames=json.loads(p.read_text())
numeric=[f for f in frames if f['bus']==1]
text=[f for f in frames if f['bus']==2]
steps=[];last=None;matched=0;overlay=0
assert len(numeric)==len(text)
for n,t in zip(numeric,text):
 assert abs(n['time']-t['time'])<.001
 b=n['mosi'];c=t['mosi']
 active=c[1]==5
 assert bool(b[6]&0x20)==active, (n,t)
 if not active:continue
 overlay+=1
 assert (b[6]&31)==c[2], (n,t)
 assert c[2]<=31 and c[3:14]==[0]*11
 matched+=1
 if c[2]!=last:
  steps.append({'time_seconds':t['time'],'volume':c[2], 'numeric_byte6_hex':f'{b[6]:02X}'})
  last=c[2]
assert [s['volume'] for s in steps]==list(range(32))
summary={'reference':'user swept volume from minimum to maximum',
 'numeric_frames':len(numeric),'text_frames':len(text),
 'overlay_frames':overlay,'cross_bus_matches':matched,
 'sequence':[s['volume'] for s in steps],
 'overlay_first_seconds':steps[0]['time_seconds'],
 'maximum_first_seconds':steps[-1]['time_seconds'],
 'fields':{'numeric_volume':'SDT1 byte6 & 0x1F',
           'numeric_overlay':'SDT1 byte6 & 0x20',
           'text_volume':'SDI2 byte1=0x05; byte2=0..31; bytes3..13=0'},
 'steps':steps}
p.with_name('volume-summary.json').write_text(json.dumps(summary,indent=2)+'\n')
with p.with_name('volume-steps.csv').open('w') as out:
 w=csv.DictWriter(out,fieldnames=list(steps[0]));w.writeheader();w.writerows(steps)
print(json.dumps({k:v for k,v in summary.items() if k!='steps'},indent=2))
