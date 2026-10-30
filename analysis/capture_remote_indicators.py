#!/usr/bin/env python3
"""Explicit controlled IR experiment. Only invoked for user-requested indicator tests."""
import json,pathlib,subprocess,sys,time,urllib.request
import serial
ROOT=pathlib.Path(__file__).resolve().parents[1]
command,stem=sys.argv[1:]
if command not in {'radio_band','tune_up','tune_down','date'}:raise SystemExit('Command outside this experiment')
p=ROOT/'captures/remote-indicators'/stem
if p.exists():raise SystemExit('Refusing to overwrite an experiment')
p.mkdir(parents=True)
u=urllib.request.build_opener(urllib.request.ProxyHandler({}))
def state():return json.load(u.open('http://192.168.4.1/api/state',timeout=4))
def call(n,**a):
 q={'jsonrpc':'2.0','id':1,'method':'tools/call','params':{'name':n,'arguments':a}}
 r=json.load(u.open(urllib.request.Request('http://127.0.0.1:10530',data=json.dumps(q).encode(),headers={'Content-Type':'application/json'}),timeout=25))['result']
 if r.get('isError'):raise RuntimeError(r)
 return r.get('structuredContent') or json.loads(next(c['text'] for c in r['content'] if c['type']=='text'))
before=state();cid=call('start_capture',deviceId='DAAD127308D7F0CD',logicDeviceConfiguration={'logicChannels':{'digitalChannels':list(range(8))},'digitalSampleRate':8000000},captureConfiguration={'timedCaptureMode':{'durationSeconds':6}})['captureId']
start=time.monotonic();time.sleep(1)
s=serial.Serial('/dev/serial/by-id/usb-Hades2001_M5stack_39522B0848-if00-port0',115200,timeout=.3);s.dtr=False;s.rts=False;s.reset_input_buffer();s.write(('send '+command+'\r').encode());s.flush();time.sleep(.4);ack=s.read(4000).decode(errors='replace');s.close()
print(command,ack.strip(),flush=True);time.sleep(3)
after=state();(p/'experiment.json').write_text(json.dumps({'command':command,'ack':ack,'before':before,'after':after},indent=2)+'\n');time.sleep(max(0,7-(time.monotonic()-start)))
call('save_capture',captureId=cid,filepath=str(p/'capture.sal'));raw=p/'raw';raw.mkdir()
call('export_raw_data_csv',captureId=cid,directory=str(raw),analogDownsampleRatio=1,logicChannels={'digitalChannels':list(range(8))})
try:call('close_capture',captureId=cid)
except Exception as exc:print('Capture saved/exported; close warning:',exc,flush=True)
record={'command':command,'ack':ack,'before':before,'after':after}
(p/'experiment.json').write_text(json.dumps(record,indent=2)+'\n')
subprocess.run(['python3',str(ROOT/'analysis/extract_frames.py'),str(raw/'digital.csv')],check=True)
print(json.dumps({'source':after['source'],'radio':after['radio'],'raw':after['buses'][0]['raw_hex'],'title':after['title']},indent=2))
