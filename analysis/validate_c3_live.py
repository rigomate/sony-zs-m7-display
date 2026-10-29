#!/usr/bin/env python3
"""Passive Saleae/API comparison. Does not transmit any player command."""
import json,pathlib,re,subprocess,time,urllib.request
ROOT=pathlib.Path(__file__).resolve().parents[1]
OUT=ROOT/'captures/tenstar-c3'
u=urllib.request.build_opener(urllib.request.ProxyHandler({}))
def mcp(name,**args):
 p={'jsonrpc':'2.0','id':1,'method':'tools/call','params':{'name':name,'arguments':args}}
 r=json.load(u.open(urllib.request.Request('http://127.0.0.1:10530',data=json.dumps(p).encode(),headers={'Content-Type':'application/json'}),timeout=25))['result']
 if r.get('isError'):raise RuntimeError(r)
 return r.get('structuredContent') or json.loads(next(c['text'] for c in r['content'] if c['type']=='text'))
duration=20
r=mcp('start_capture',deviceId='DAAD127308D7F0CD',logicDeviceConfiguration={'logicChannels':{'digitalChannels':list(range(8))},'digitalSampleRate':8000000},captureConfiguration={'timedCaptureMode':{'durationSeconds':duration}})
cid=r['captureId'];start=time.monotonic();rows=[]
while time.monotonic()-start<duration-1:
 state=json.load(u.open('http://192.168.4.1/api/state',timeout=5))
 rows.append({'at':time.monotonic()-start,'state':state})
 if len(rows)%16==0:u.open('http://192.168.4.1/',timeout=5).read()
 time.sleep(.25)
remaining=duration+1-(time.monotonic()-start)
if remaining>0:time.sleep(remaining)
stem='04-payload-comparison'
mcp('save_capture',captureId=cid,filepath=str(OUT/(stem+'.sal')))
raw=OUT/(stem+'-raw');raw.mkdir(exist_ok=True)
mcp('export_raw_data_csv',captureId=cid,directory=str(raw),analogDownsampleRatio=1,logicChannels={'digitalChannels':list(range(8))})
mcp('close_capture',captureId=cid)
(OUT/(stem+'-api.jsonl')).write_text(''.join(json.dumps(r)+'\n' for r in rows))
output=subprocess.check_output([str(ROOT/'build/display_replay'),str(raw/'digital.csv'),'--dump'],text=True)
wire=[set(),set()]
for line in output.splitlines():
 if 's SPI-' not in line or '[partial' in line:continue
 index=0 if 's SPI-1' in line else 1
 payload=bytes(int(v,16) for v in re.findall(r'0x([0-9A-F]{2})',line.split(' | ')[0]))
 wire[index].add(payload)
results=[]
for i,length in [(0,16),(1,15)]:
 matched=0;unmatched=[];partial=0
 for sample in rows:
  if sample['at']<1 or sample['at']>duration-2:continue
  state=sample['state'];payload=bytes.fromhex(state['buses'][i]['raw_hex'])
  if len(payload)!=length:partial+=1;continue
  if payload in wire[i]:matched+=1
  else:unmatched.append({'at':sample['at'],'hex':payload.hex(' '),'numeric':state['numeric'],'title':state['title']})
 a,b=rows[0]['state']['buses'][i],rows[-1]['state']['buses'][i]
 results.append({'bus':i+1,'exact_payload_matches':matched,'unmatched':unmatched,'partial_samples':partial,
                 'frames':b['frames']-a['frames'],'complete':b['complete']-a['complete'],'rejected':b['rejected']-a['rejected']})
summary={'samples':len(rows),'duration':duration,'queue_drops_delta':rows[-1]['state']['dropped_frames']-rows[0]['state']['dropped_frames'],'results':results,'first':rows[0]['state']['numeric'],'last':rows[-1]['state']['numeric']}
(OUT/(stem+'-summary.json')).write_text(json.dumps(summary,indent=2)+'\n')
print(json.dumps(summary,indent=2))
