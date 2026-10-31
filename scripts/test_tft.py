#!/usr/bin/env python3
"""Exercise the 320×240 browser display against live fixtures and example modes."""
import copy
from http.server import BaseHTTPRequestHandler, HTTPServer
import json
from pathlib import Path
import subprocess
import tempfile
import threading
from urllib.parse import urlsplit

root=Path(__file__).resolve().parents[1]
page=(root/'firmware/esp32/web/tft.html').read_text()
audit='''<script>setTimeout(()=>{const s=document.getElementById('screen');const bounds=s.getBoundingClientRect();let problems=[];for(const el of s.querySelectorAll('.source,.tag,.hero,.metric-value,.calendar,.message,.heading-line')){const r=el.getBoundingClientRect();if(r.left<bounds.left-1||r.right>bounds.right+1||r.top<bounds.top-1||r.bottom>bounds.bottom+1||el.scrollWidth>el.clientWidth+1)problems.push(el.className)}s.dataset.auditSize=s.offsetWidth+'x'+s.offsetHeight;s.dataset.auditOverflow=problems.join(',')||'none';},600);</script>'''
page=page.replace('</body>',audit+'</body>').encode()
base={'source':'md','mode':'md','buses':[{'fresh':True},{'fresh':True}],
      'symbols':{'md':True,'md_play':True},'numeric':{'track':3,'minutes':2,'seconds':37,'indicators':1048572},
      'track_display':{'number':3,'indicators':1048572,'playing':True},
      'md':{'play':True,'pause':False,'time_blank':False},'title':{'text':'Midnight Drive'}}
active=base
class Handler(BaseHTTPRequestHandler):
 def log_message(self,*args):pass
 def do_GET(self):
  path=urlsplit(self.path).path
  if path=='/tft': data,mime=page,'text/html; charset=utf-8'
  elif path=='/api/state':data,mime=json.dumps(active).encode(),'application/json'
  else:self.send_error(404);return
  self.send_response(200);self.send_header('Content-Type',mime);self.send_header('Content-Length',str(len(data)));self.end_headers();self.wfile.write(data)
server=HTTPServer(('127.0.0.1',0),Handler)
threading.Thread(target=server.serve_forever,daemon=True).start()
def check(name,url,expected,absent=()):
 with tempfile.TemporaryDirectory(prefix='zs-tft-test-') as profile:
  result=subprocess.run(['chromium','--headless','--no-sandbox','--disable-gpu','--disable-dev-shm-usage',
      '--user-data-dir='+profile,'--window-size=1024,900','--virtual-time-budget=1200','--dump-dom',
      'http://127.0.0.1:'+str(server.server_port)+url],capture_output=True,text=True,check=True,timeout=25)
  dom=result.stdout.split('<script>')[0]
  for item in ('data-audit-size="320x240"','data-audit-overflow="none"',*expected):
   if item not in dom:raise AssertionError(name+': missing '+item+'; '+dom[dom.find('id="screen"'):dom.find('id="screen"')+1500])
  for item in absent:
   if item in dom:raise AssertionError(name+': unexpected '+item)
  print('PASS TFT:',name,flush=True)
try:
 cases={'md':['MINIDISC','Midnight Drive','02:37','class="calendar"'],
        'paused':['PAUSE','Midnight Drive'], 'nodisc':['No Disc'], 'cd':['CD source selected'],
        'line':['Line input'], 'fm':['104.00','MHz','PRESET'], 'am':['1287','kHz'],
        'record':['Recording','70:19','REC','class="calendar"'], 'clock':['18','55','STANDBY','class="md-icon"','TIMER','PLAY'],
        'clockedit':['Set clock','Adjust minute'], 'timermode':['Timer setup','TIMER','PLAY','Choose PLAY or REC'],
        'timerplaysource':['TIMER PLAY','Wake-up source','RADIO'],
        'timerplaycd':['TIMER PLAY','Wake-up source','CD'],
        'timerplaymd':['TIMER PLAY','Wake-up source','MD'],
        'timerplayline':['TIMER PLAY','Wake-up source','LINE'],
        'timerrec':['Timer setup','TIMER','REC'],
        'timerrecsource':['TIMER REC','Recording source','RADIO'],
        'timerrecstart':['TIMER REC','Record start','19','09'],
        'timerrecstop':['TIMER REC','Record stop','19','12'],
        'timerrecvolume':['TIMER REC','Timer volume','20'],
        'timerstart':['Timer start','Adjust minute'],
        'timerstop':['Timer stop','Adjust hour'], 'timervolume':['Timer volume','21'],
        'sleep':['Sleep timer','30'], 'program':['Program track','07'], 'volume':['Volume','18'],
        'bass':['Bass','/ 12'], 'offline':['No signal']}
 for name,expected in cases.items():check(name,'/tft?demo='+name,expected)
 check('live MD','/tft',['Midnight Drive','02:37','Live · 320 × 240'])
 active=copy.deepcopy(base);active['clock_setting']={'hour':0,'minute':0,'editing':'hour','hour_blank':False,'minute_blank':False,'fresh':True}
 check('clock setting midnight hour lit','/tft',['Set clock','Adjust hour','<span class="">00</span>'])
 active['clock_setting']['hour_blank']=True
 check('clock setting midnight hour blank','/tft',['Set clock','Adjust hour','<span class="blank">00</span>'])
 active=copy.deepcopy(base);active['symbols'].update(timer_rec=True,repeat_one=True)
 check('MD with timer recording armed','/tft',['MINIDISC','TIMER REC','Midnight Drive'])
 active=copy.deepcopy(base);active.update(source='unknown',numeric=None,track_display=None,md=None,title=None,
     symbols={'md_disc':'solid','timer':True,'timer_play':True},
     clock_display={'hour':0,'minute':12,'colon_on':True,'standby':True,'fresh':True})
 check('live standby midnight with MD','/tft',['STANDBY','Standby clock','00','12','class="md-icon"','TIMER','PLAY','MiniDisc indicator on'])
 active['symbols']['md_disc']='off'
 check('live standby MD icon off','/tft',['STANDBY','Standby clock'],['class="md-icon"'])
 active=copy.deepcopy(base);active.update(source='unknown',numeric=None,track_display=None,md=None,
     title={'text':'26y 10m 31d '},symbols={'date':True,'timer':True,'timer_play':True},
     clock_display={'hour':9,'minute':4,'colon_on':True,'standby':False,'fresh':True})
 check('clock date and time','/tft',['CLOCK','<span class="">09</span>','<span class="">04</span>','26y 10m 31d'])
 active['clock_display']['colon_on']=False
 check('clock date and blinking colon','/tft',['26y 10m 31d','<span class="blank">:</span>'])
 active=copy.deepcopy(base);active.update(source='unknown',numeric=None,track_display=None,md=None,title=None,
     symbols={'timer':True,'timer_play':True},timer_mode_selection={'choice':'play','timer_on':True,'choice_on':True,'fresh':True})
 check('live timer play lit','/tft',['Timer setup','<span class="">TIMER</span> <span class="">PLAY</span>','Turn dial · Press ENTER'],['Waiting for display'])
 active['timer_mode_selection'].update(timer_on=False,choice_on=False)
 check('live timer play blank','/tft',['Timer setup','<span class="blank">TIMER</span> <span class="blank">PLAY</span>'],['Waiting for display'])
 active['timer_mode_selection'].update(choice='rec',timer_on=True,choice_on=True)
 check('live timer rec lit','/tft',['Timer setup','<span class="">TIMER</span> <span class="">REC</span>'],['Waiting for display'])
 active=copy.deepcopy(base);active.update(source='radio',numeric=None,track_display=None,md=None,title=None,
     symbols={'timer_rec':True},timer_source_selection={'mode':'rec','source':'radio','source_on':True,'fresh':True})
 check('timer rec radio source lit','/tft',['TIMER REC','Recording source','class="message ">RADIO'],['Waiting for frequency'])
 active['timer_source_selection']['source_on']=False
 check('timer rec radio source blank','/tft',['TIMER REC','Recording source','class="message blank">RADIO'],['Waiting for display'])
 active['timer_source_selection'].update(source='line',source_on=True)
 active['timer_workflow']={'stage':'select_source','mode':'rec','source':'line','editing':'unknown','fresh':True}
 check('timer rec LINE source lit','/tft',['TIMER REC','Recording source','class="message ">LINE'],['Waiting for display'])
 active['timer_source_selection']['source_on']=False
 check('timer rec LINE source blank','/tft',['TIMER REC','Recording source','class="message blank">LINE'],['Waiting for display'])
 active.pop('timer_workflow');active['timer_source_selection'].update(mode='play',source='radio',source_on=True)
 active['symbols']={'timer_play':True}
 check('wake timer radio source lit','/tft',['TIMER PLAY','Wake-up source','class="message ">RADIO'],['Waiting for frequency'])
 active['timer_source_selection']['source_on']=False
 check('wake timer radio source blank','/tft',['TIMER PLAY','Wake-up source','class="message blank">RADIO'],['Waiting for display'])
 active['timer_source_selection'].update(source='cd',source_on=True)
 check('wake timer CD source lit','/tft',['TIMER PLAY','Wake-up source','class="message ">CD'],['Waiting for display'])
 active['timer_source_selection']['source_on']=False
 check('wake timer CD source blank','/tft',['TIMER PLAY','Wake-up source','class="message blank">CD'],['Waiting for display'])
 active['timer_source_selection'].update(source='md',source_on=True)
 check('wake timer MD source lit','/tft',['TIMER PLAY','Wake-up source','class="message ">MD'],['Waiting for display'])
 active['timer_source_selection']['source_on']=False
 check('wake timer MD source blank','/tft',['TIMER PLAY','Wake-up source','class="message blank">MD'],['Waiting for display'])
 active['timer_workflow']={'stage':'select_source','mode':'play','source':'md','editing':'unknown','fresh':True}
 check('workflow wake MD source blank','/tft',['data-timer-stage="select_source"','Wake-up source','class="message blank">MD'],['Waiting for display'])
 active['timer_source_selection'].update(source='line',source_on=True);active['timer_workflow']['source']='line'
 check('workflow wake LINE source lit','/tft',['data-timer-stage="select_source"','Wake-up source','class="message ">LINE'],['Waiting for display'])
 active['timer_source_selection']['source_on']=False
 check('workflow wake LINE source blank','/tft',['data-timer-stage="select_source"','Wake-up source','class="message blank">LINE'],['Waiting for display'])
 active['timer_source_selection']=None
 active['timer_workflow'].update(stage='start',source='md',editing='hour')
 active['timer_setting']={'phase':'on','hour':16,'minute':31,'editing':'hour','hour_blank':True,'minute_blank':False,'fresh':True}
 check('workflow timer start','/tft',['data-timer-stage="start"','Timer start','<span class="blank">16</span>'])
 active['timer_setting']=None;active['timer_workflow'].update(stage='volume',editing='unknown')
 active['timer_volume_setting']={'level':20,'blank':False,'fresh':True}
 check('workflow timer volume','/tft',['data-timer-stage="volume"','Timer volume','20'])
 active['timer_workflow']={'stage':'unknown','mode':'unknown','source':'unknown','editing':'unknown','fresh':True}
 active['timer_volume_setting']=None;active['timer_source_selection']={'mode':'play','source':'md','source_on':True,'fresh':True}
 check('unknown workflow suppresses stale source','/tft',['data-timer-stage="unknown"'],['Wake-up source'])
 active.pop('timer_workflow');active['timer_source_selection']=None
 active['symbols']={'timer_rec':True}
 active['timer_setting']={'phase':'on','hour':19,'minute':9,'editing':'hour','hour_blank':True,'minute_blank':False,'fresh':True}
 check('timer rec start hour','/tft',['TIMER REC','Record start','<span class="blank">19</span>','09'])
 active['timer_setting'].update(phase='off',minute=12,editing='minute',hour_blank=False,minute_blank=True)
 check('timer rec stop minute','/tft',['TIMER REC','Record stop','<span class="blank">12</span>'])
 active['timer_setting']=None;active['timer_volume_setting']={'level':20,'blank':False,'fresh':True}
 check('timer rec volume visible','/tft',['TIMER REC','Timer volume','20'])
 active['timer_volume_setting']['blank']=True
 check('timer rec volume blank','/tft',['TIMER REC','Timer volume','hero blank">20'])
 active=copy.deepcopy(base);active.update(source='radio',numeric=None,track_display=None,md=None,radio=None,
     symbols={'radio':True,'timer_play':True},
     timer_setting={'phase':'on','hour':19,'minute':9,'editing':'hour','hour_blank':False,'minute_blank':False,'fresh':True})
 check('radio wake start hour lit','/tft',['TIMER','Timer start','<span class="">19</span>','<span class="">09</span>'])
 active['timer_setting']['hour_blank']=True
 check('radio wake start hour blank','/tft',['Timer start','<span class="blank">19</span>','<span class="">09</span>'],['Waiting for frequency'])
 active['timer_setting']=None;active['timer_volume_setting']={'level':20,'blank':False,'fresh':True}
 check('radio wake volume visible','/tft',['Timer volume','20'],['Waiting for frequency'])
 active['timer_volume_setting']['blank']=True
 check('radio wake volume blank','/tft',['Timer volume','hero blank">20'],['Waiting for frequency'])
 active.update(source='cd',symbols={'cd':True,'timer_play':True})
 active['timer_volume_setting']['blank']=False
 check('CD wake volume visible','/tft',['Timer volume','20'],['Waiting for display'])
 active['timer_volume_setting']['blank']=True
 check('CD wake volume blank','/tft',['Timer volume','hero blank">20'],['Waiting for display'])
 active=copy.deepcopy(base);active['timer_setting']={'phase':'on','hour':16,'minute':31,'editing':'minute','hour_blank':False,'minute_blank':True,'fresh':True}
 check('live timer minute blank','/tft',['Timer start','Adjust minute','class="blank">31'],['Example · not live'])
 active=copy.deepcopy(base);active['title']={'text':'<img src=x>'}
 check('title escaping','/tft',['&lt;img src=x&gt;'],['<img src="x">'])
 print('All compact display checks passed',flush=True)
finally:server.shutdown()
