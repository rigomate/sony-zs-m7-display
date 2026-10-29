#!/usr/bin/env python3
"""Explicitly invoked hardware measurements. Only play/track_next are sent."""
import datetime,json,pathlib,sys,time,urllib.request
import serial
ROOT=pathlib.Path(__file__).resolve().parents[1]
OUT=ROOT/'captures/test20'
OUT.mkdir(exist_ok=True)

def log(event,**kw):
 record={'utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),'event':event,**kw}
 with (OUT/'run.jsonl').open('a') as f:f.write(json.dumps(record)+'\n')
 print(json.dumps(record),flush=True)

def call(name,**args):
 p={'jsonrpc':'2.0','id':1,'method':'tools/call','params':{'name':name,'arguments':args}}
 req=urllib.request.Request('http://127.0.0.1:10530',data=json.dumps(p).encode(),headers={'Content-Type':'application/json'})
 r=json.load(urllib.request.urlopen(req,timeout=25))
 if 'error' in r:raise RuntimeError(r)
 r=r['result']
 if r.get('isError'):raise RuntimeError(r)
 if 'structuredContent' in r:return r['structuredContent']
 return json.loads(next(c['text'] for c in r['content'] if c['type']=='text'))

def send(command):
 assert command in ('play','track_next')
 with serial.Serial('/dev/ttyUSB0',115200,timeout=1) as s:
  s.reset_input_buffer();s.write(('send '+command+'\r').encode());s.flush()
  reply=s.read(2000).decode(errors='replace')
 log('remote_ack',command=command,reply=reply)
 if 'OK' not in reply:raise RuntimeError('No ACK; do not blindly retry')
 time.sleep(3)

def start(stem,duration):
 if (OUT/(stem+'.sal')).exists() or (OUT/(stem+'-raw')).exists():
  raise RuntimeError('Capture already exists; choose a new run directory before transmitting')
 r=call('start_capture',deviceId='DAAD127308D7F0CD',logicDeviceConfiguration={'logicChannels':{'digitalChannels':list(range(8))},'digitalSampleRate':8000000},captureConfiguration={'timedCaptureMode':{'durationSeconds':duration}})
 log('capture_started',stem=stem,duration=duration,response=r)
 return r['captureId']

def save(cid,stem):
 log('saved',stem=stem,response=call('save_capture',captureId=cid,filepath=str(OUT/(stem+'.sal'))))
 d=OUT/(stem+'-raw');d.mkdir(exist_ok=True)
 log('exported',stem=stem,response=call('export_raw_data_csv',captureId=cid,directory=str(d),analogDownsampleRatio=1,logicChannels={'digitalChannels':list(range(8))}))
 call('close_capture',captureId=cid)

if __name__=='__main__':
 mode=sys.argv[1]
 if mode=='begin':
  stem='01-play-time';cid=start(stem,140)
  (OUT/'active.json').write_text(json.dumps({'captureId':cid,'stem':stem,'start_wall':time.time()}))
  send('play')
 elif mode=='save':
  a=json.loads((OUT/'active.json').read_text());save(a['captureId'],a['stem']);(OUT/'active.json').unlink()
 elif mode=='sweep':
  for n in range(2,21):
   stem=f'{n:02d}-next';cid=start(stem,16)
   send('track_next');time.sleep(13)
   save(cid,stem)
 else:raise ValueError(mode)
