#!/usr/bin/env python3
"""Explicit remote/capture runner for the user-requested stopped/track21 test."""
import pathlib,json,urllib.request,subprocess,time,sys
import serial
ROOT=pathlib.Path(__file__).resolve().parents[1];stem=sys.argv[1];commands=sys.argv[2:]
allowed={'play','greater_than_10','digit_0','digit_1','digit_2','track_next','track_prev','disc_name'}
if any(c not in allowed for c in commands):raise SystemExit('Command not allowed for this experiment')
p=ROOT/'captures/track21'/stem
if p.exists():raise SystemExit('Refusing to overwrite capture')
p.mkdir(parents=True);u=urllib.request.build_opener(urllib.request.ProxyHandler({}))
def state():return json.load(u.open('http://192.168.4.1/api/state',timeout=4))
def call(n,**a):
 q={'jsonrpc':'2.0','id':1,'method':'tools/call','params':{'name':n,'arguments':a}}
 r=json.load(u.open(urllib.request.Request('http://127.0.0.1:10530',data=json.dumps(q).encode(),headers={'Content-Type':'application/json'}),timeout=25))['result']
 if r.get('isError'):raise RuntimeError(r)
 return r.get('structuredContent') or json.loads(next(c['text'] for c in r['content'] if c['type']=='text'))
before=state();duration=max(6,len(commands)*4+3)
cid=call('start_capture',deviceId='DAAD127308D7F0CD',logicDeviceConfiguration={'logicChannels':{'digitalChannels':list(range(8))},'digitalSampleRate':8000000},captureConfiguration={'timedCaptureMode':{'durationSeconds':duration}})['captureId']
start=time.monotonic();log=[];time.sleep(1)
for command in commands:
 s=serial.Serial('/dev/serial/by-id/usb-Hades2001_M5stack_39522B0848-if00-port0',115200,timeout=.3);s.dtr=False;s.rts=False;s.reset_input_buffer();s.write(('send '+command+'\r').encode());s.flush();time.sleep(.4);ack=s.read(4000).decode(errors='replace');s.close();time.sleep(3)
 observed=state();log.append({'command':command,'ack':ack,'state':observed});print(command,ack.strip(),'raw',observed['buses'][0]['raw_hex'],flush=True)
after=state();(p/'experiment.json').write_text(json.dumps({'before':before,'commands':log,'after':after},indent=2)+'\n');time.sleep(max(0,duration+1-(time.monotonic()-start)))
call('save_capture',captureId=cid,filepath=str(p/'capture.sal'));raw=p/'raw';raw.mkdir()
call('export_raw_data_csv',captureId=cid,directory=str(raw),analogDownsampleRatio=1,logicChannels={'digitalChannels':list(range(8))})
try:call('close_capture',captureId=cid)
except Exception as exc:print('Saved/exported; close warning:',exc)
subprocess.run(['python3',str(ROOT/'analysis/extract_frames.py'),str(raw/'digital.csv')],check=True)
print('Result:',after['buses'][0]['raw_hex'])
