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
    render('live capture', ['class="status live"', 'id="track">20', 'id="time">00:14',
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
    active = copy.deepcopy(base)
    active['numeric'] = None
    active['radio'] = {'frequency_khz': 104000, 'band': 'FM', 'preset': True}
    render('FM preset', ['FM RADIO', 'Frequency · MHz', 'id="track">104.00',
                         'id="preset" class="chip">PRESET', 'id="elapsed-group" hidden=""',
                         'id="symbol-mhz" class="symbol on"',
                         'id="symbol-khz" class="symbol off"',
                         'id="symbol-preset" class="symbol on"'])
    active['radio'] = {'frequency_khz': 1287, 'band': 'AM', 'preset': False}
    render('AM units and preset off', ['AM RADIO', 'Frequency · kHz', 'id="track">1287',
                                      'id="preset" class="chip" hidden=""',
                                      'id="symbol-khz" class="symbol on"',
                                      'id="symbol-mhz" class="symbol off"',
                                      'id="symbol-preset" class="symbol off"'])
    active = copy.deepcopy(base)
    render('return to MD', ['MINIDISC', 'id="track">20', 'id="time">00:14'])
    active = copy.deepcopy(base)
    active.setdefault('symbols',{})['remain'] = True
    render('MD remaining playback', ['id="elapsed-group-label" class="eyebrow">Remaining', 'id="time">00:14'])
    active['symbols']['remain'] = False
    render('MD elapsed playback', ['id="elapsed-group-label" class="eyebrow">Elapsed', 'id="time">00:14'])
    active = copy.deepcopy(base)
    active['md'] = {'play': True, 'pause': False, 'disc': 'animated', 'time_blank': False}
    render('MD animation', ['disc-animated', 'id="symbol-md-play" class="symbol on"',
                            'disc-animated .wedge:nth-of-type(2){animation-delay:-1s}',
                            'disc-animated .wedge:nth-of-type(3){animation-delay:-.5s}',
                            'id="symbol-md-pause" class="symbol off"'])
    active['md'] = {'play': True, 'pause': True, 'disc': 'solid', 'time_blank': True}
    render('MD paused', ['disc-solid', 'id="symbol-md-pause" class="symbol on"'], ['class="symbol on disc-animated"'])
    active = copy.deepcopy(base)
    active['source'] = 'md'
    active['symbols'] = copy.deepcopy(base.get('symbols') or {})
    active['symbols'].update(md_disc='animated-reverse-fast', md_play=True, md_pause=False)
    active['md'] = {'play': True, 'pause': False, 'disc': 'animated-reverse-fast', 'time_blank': False}
    render('MD fast reverse disc', ['id="symbol-md-disc" class="symbol on disc-animated-reverse-fast"',
                                   'disc-animated-reverse-fast .wedge:nth-of-type(2){animation-delay:-.2s}',
                                   'disc-animated-reverse-fast .wedge:nth-of-type(3){animation-delay:-.1s}'])
    active['symbols']['md_disc'] = 'animated-forward-fast'
    active['md']['disc'] = 'animated-forward-fast'
    render('MD fast forward disc', ['id="symbol-md-disc" class="symbol on disc-animated-forward-fast"',
                                   'disc-animated-forward-fast .wedge:nth-of-type(2){animation-delay:-.1s}',
                                   'disc-animated-forward-fast .wedge:nth-of-type(3){animation-delay:-.2s}'])
    active = copy.deepcopy(base)
    active['volume'] = {'level': 0, 'overlay': True, 'fresh': True}
    active['title'] = None
    render('volume minimum', ['Volume display · 0 / 31', 'Volume: 0', 'id="volume-value" class="volume-value">0 / 31'],
           ['Display transition', 'Title: transition'])
    active['volume']['level'] = 31
    render('volume maximum', ['Volume display · 31 / 31', 'aria-valuenow="31"', 'width: 100%'])
    active = copy.deepcopy(base)
    active['volume'] = {'level': 31, 'overlay': False, 'fresh': True}
    render('title restored after volume', ['12-character display window', '31 / 31'], ['Volume display ·'])
    active = copy.deepcopy(base)
    active['audio_adjustment'] = {'position': 0, 'steps': 12, 'setting': 'bass', 'fresh': True}
    render('BASS adjustment minimum', ['BASS adjustment', '0 / 12'],
           ['id="adjustment-readout" class="adjustment-readout stale" hidden'])
    active['audio_adjustment']['position'] = 12
    render('BASS adjustment maximum', ['BASS adjustment', '12 / 12'])
    active['audio_adjustment'] = {'position': 6, 'steps': 12, 'setting': 'treble', 'fresh': True}
    render('TRE adjustment', ['TRE adjustment', '6 / 12'])
    active = copy.deepcopy(base)
    active['audio_adjustment'] = None
    render('ordinary display without adjustment', ['class="adjustment-readout stale" hidden'])
    active = copy.deepcopy(base)
    active['sleep_timer'] = {'minutes': None, 'fresh': True}
    render('sleep timer blank phase', ['id="number-label" class="eyebrow">Sleep timer',
                                      'id="track">—', 'id="elapsed-group" hidden=""',
                                      'id="tracks-label" class="tracks-label" hidden=""'])
    active['sleep_timer'] = {'minutes': 10, 'fresh': True}
    render('sleep timer ten minutes', ['Sleep timer', 'id="track">10 min'])
    active['sleep_timer'] = {'minutes': 120, 'fresh': True}
    render('sleep timer one hundred twenty minutes', ['id="track">120 min'])
    active = copy.deepcopy(base)
    active.update(source='radio', mode='radio', numeric=None, track_display=None,
                  radio={'frequency_khz': 103900, 'band': 'FM', 'display_band': 'FM', 'preset': False},
                  sleep_timer={'minutes': 90, 'fresh': True})
    active['symbols'] = copy.deepcopy(base.get('symbols') or {})
    active['symbols']['sleep'] = True
    render('sleep timer on radio source', ['FM RADIO', 'id="number-label" class="eyebrow">Sleep timer',
                                          'id="track">90 min', 'id="symbol-sleep" class="symbol on"',
                                          'id="elapsed-group" hidden=""'])
    active = copy.deepcopy(base)
    active['sleep_timer'] = None
    render('sleep timer mode ended', ['id="number-label" class="eyebrow">Track'],
           ['id="number-label" class="eyebrow">Sleep timer'])
    active = copy.deepcopy(base)
    active['clock_setting'] = {'hour': 7, 'minute': 0, 'editing': 'hour', 'fresh': True}
    render('clock hour setting', ['id="number-label" class="eyebrow">Clock setting · hour',
                                 'aria-label="Clock setting 07:00, editing hour">07:00', 'id="elapsed-group" hidden=""',
                                 'id="tracks-label" class="tracks-label" hidden=""'])
    active['clock_setting'] = {'hour': 6, 'minute': 52, 'editing': 'hour', 'fresh': True}
    render('clock hour setting during playback', ['Clock setting · hour', 'aria-label="Clock setting 06:52, editing hour">06:52'])
    active['clock_setting']['hour_blank'] = True
    render('clock hour blank phase', ['aria-label="Clock setting 06:52, editing hour">0<span class="clock-blank">6</span>:52'])
    active['clock_setting']={'hour':0,'minute':0,'editing':'hour','hour_blank':True,'minute_blank':False,'fresh':True}
    render('clock midnight hour blank', ['aria-label="Clock setting 00:00, editing hour">0<span class="clock-blank">0</span>:00'])
    active['clock_setting'] = {'hour': 23, 'minute': 59, 'editing': 'minute', 'fresh': True}
    render('clock minute setting', ['Clock setting · minute', 'aria-label="Clock setting 23:59, editing minute">23:59'])
    active['clock_setting']['minute_blank'] = True
    render('clock minute blank phase', ['aria-label="Clock setting 23:59, editing minute">23:<span class="clock-blank">59</span>'])
    active['clock_setting'] = None
    render('clock setting ended', ['id="number-label" class="eyebrow">Track'],
           ['Clock setting · minute'])
    active['timer_setting'] = {'hour': 16, 'minute': 31, 'editing': 'hour', 'hour_blank': False, 'minute_blank': False, 'fresh': True}
    render('timer on hour visible', ['Timer start · hour', 'aria-label="Timer start 16:31, editing hour">16:31',
           'id="elapsed-group" hidden=""'])
    active['timer_setting']['hour_blank'] = True
    render('timer on hour blank phase', ['Timer start · hour',
           'aria-label="Timer start 16:31, editing hour"><span class="clock-blank">16</span>:31'])
    active['timer_setting'].update(editing='minute',hour_blank=False,minute_blank=False)
    render('timer on minute visible', ['Timer start · minute', 'aria-label="Timer start 16:31, editing minute">16:31'])
    active['timer_setting']['minute_blank'] = True
    render('timer on minute blank phase', ['Timer start · minute',
           'aria-label="Timer start 16:31, editing minute">16:<span class="clock-blank">31</span>'])
    active['timer_setting']['phase'] = 'off'
    render('timer stop minute', ['Timer stop · minute', 'aria-label="Timer stop 16:31, editing minute"'])
    active['timer_setting'] = None
    render('timer on setting ended', ['id="number-label" class="eyebrow">Track'], ['Timer start · hour'])
    active=copy.deepcopy(base)
    active.update(source='radio',radio=None,numeric=None,track_display=None,md=None,
                  symbols={'radio':True,'timer_play':True},
                  timer_setting={'phase':'on','hour':19,'minute':9,'editing':'hour','hour_blank':False,'minute_blank':False,'fresh':True})
    render('radio wake start hour lit', ['Timer start · hour', 'aria-label="Timer start 19:09, editing hour">19:09'])
    active['timer_setting']['hour_blank']=True
    render('radio wake start hour blank', ['Timer start · hour',
           'aria-label="Timer start 19:09, editing hour"><span class="clock-blank">19</span>:09'])
    active = copy.deepcopy(base)
    active['timer_volume_setting'] = {'level': 21, 'blank': False, 'fresh': True}
    active['volume'] = {'level': 21, 'overlay': False, 'fresh': True}
    render('timer volume visible', ['Timer volume', 'id="track" aria-label="Timer volume 21 / 31">21 / 31', 'id="elapsed-group" hidden=""'])
    active['timer_volume_setting']['blank'] = True
    render('timer volume blank', ['id="track" aria-label="Timer volume 21 / 31">&nbsp;'])
    active=copy.deepcopy(base)
    active.update(source='radio',radio=None,numeric=None,track_display=None,md=None,
                  symbols={'radio':True,'timer_play':True},
                  timer_volume_setting={'level':20,'blank':False,'fresh':True},
                  volume={'level':20,'overlay':False,'fresh':True})
    render('radio wake volume visible', ['Timer volume', 'id="track" aria-label="Timer volume 20 / 31">20 / 31'])
    active['timer_volume_setting']['blank']=True
    render('radio wake volume blank', ['id="track" aria-label="Timer volume 20 / 31">&nbsp;'])
    active.update(source='cd',symbols={'cd':True,'timer_play':True})
    active['timer_volume_setting']['blank']=False
    render('CD wake volume visible', ['Timer volume','id="track" aria-label="Timer volume 20 / 31">20 / 31'])
    active['timer_volume_setting']['blank']=True
    render('CD wake volume blank', ['id="track" aria-label="Timer volume 20 / 31">&nbsp;'])
    active = copy.deepcopy(base)
    active['clock_display'] = {'hour': 18, 'minute': 55, 'colon_on': True, 'fresh': True}
    render('clock readout colon lit', ['id="number-label" class="eyebrow">Clock', 'aria-label="Clock 18:55">18<span class="clock-colon">:</span>55', 'id="elapsed-group" hidden=""'])
    active['clock_display']['colon_on'] = False
    render('clock readout colon dark', ['aria-label="Clock 18:55">18<span class="clock-colon clock-blank">:</span>55'])
    active = copy.deepcopy(base)
    active.update(source='unknown',numeric=None,md=None,track_display=None)
    active['clock_display'] = {'hour': 19, 'minute': 8, 'colon_on': True, 'standby': True, 'fresh': True}
    active['symbols'] = {'md': False, 'cd': False, 'radio': False, 'line': False, 'md_disc': 'solid', 'timer': True, 'timer_play': True}
    render('standby clock and MD icon', ['STANDBY', 'Standby clock', 'aria-label="Standby clock 19:08">19<span class="clock-colon">:</span>08', 'id="symbol-md-disc" class="symbol on disc-solid"', 'class="status live"'], ['disc-animated"', 'Check numeric data'])
    active['clock_display'].update(hour=0,minute=12)
    render('standby single-digit clock and MD icon', ['STANDBY', 'aria-label="Standby clock 00:12">00<span class="clock-colon">:</span>12',
           'id="symbol-md-disc" class="symbol on disc-solid"', 'id="symbol-timer" class="symbol boxed on"',
           'id="symbol-timer-play" class="symbol boxed on"'])
    for source,label in [('md','MINIDISC'),('cd','CD'),('radio','RADIO'),('line','LINE IN')]:
        active = copy.deepcopy(base)
        active.update(source=source, numeric=None, radio=None, md=None)
        active['symbols'] = {k:k==source for k in ['md','cd','radio','line']}
        active['symbols'].update(md_disc='off', md_play=False, md_pause=False)
        render('empty source '+source, ['id="source" class="chip">'+label,
                'id="symbol-'+source+'" class="symbol boxed on"',
                'id="symbol-md-disc" class="symbol off"'], ['Check numeric data'])
    active['source']='radio'
    active['symbols'].update(line=False, radio=True)
    active['radio']={'frequency_khz':104000,'band':'FM','preset':False}
    render('FM without MD', ['id="track">104.00', 'id="symbol-md-disc" class="symbol off"',
                            'id="symbol-mhz" class="symbol on"'])
    active['symbols']['sleep']=True
    render('sleep steady on', ['id="symbol-sleep" class="symbol on"'])
    active['symbols']['sleep']=False
    render('sleep off', ['id="symbol-sleep" class="symbol off"'])
    active['symbols'].update(timer=True,timer_play=True,timer_rec=False)
    render('timer play lit phase', ['id="symbol-timer" class="symbol boxed on"',
                                    'id="symbol-timer-play" class="symbol boxed on"',
                                    'id="symbol-timer-rec" class="symbol boxed rec off"'])
    active['symbols'].update(timer=False,timer_play=False)
    render('timer play dark phase', ['id="symbol-timer" class="symbol boxed off"',
                                     'id="symbol-timer-play" class="symbol boxed off"'])
    active['symbols'].update(timer=True,timer_play=False,timer_rec=True)
    render('timer rec lit phase', ['id="symbol-timer" class="symbol boxed on"',
           'id="symbol-timer-rec" class="symbol boxed rec on"',
           'id="symbol-timer-play" class="symbol boxed off"'])
    active['symbols'].update(timer=False,timer_rec=False)
    render('timer rec dark phase', ['id="symbol-timer" class="symbol boxed off"',
           'id="symbol-timer-rec" class="symbol boxed rec off"'])
    active['symbols'].update(timer=True,timer_play=False,timer_rec=False)
    render('timer edge phase', ['id="symbol-timer" class="symbol boxed on"',
           'id="symbol-timer-play" class="symbol boxed off"',
           'id="symbol-timer-rec" class="symbol boxed rec off"'])
    active['symbols'].update(timer=True,timer_play=True,clock=True,clock_inferred=True)
    render('timer play and clock on', ['id="symbol-timer" class="symbol boxed on"',
           'id="symbol-timer-play" class="symbol boxed on"',
           'id="symbol-clock" class="symbol on candidate"', 'Clock: on · inferred from clock-on capture'])
    active['symbols']['clock']=False
    render('clock candidate off', ['id="symbol-clock" class="symbol off candidate"'])
    active['symbols']['date']=True
    render('date on', ['id="symbol-date" class="symbol on"'])
    active['symbols']['date']=False
    render('date off', ['id="symbol-date" class="symbol off"'])
    active = copy.deepcopy(base)
    active['source']='radio'
    active['numeric']=None
    active['radio']={'frequency_khz':87500,'band':'FM','display_band':'FM','preset':False}
    active['symbols']={'fm':True}
    active['title']={'text':None,'codes':[141,139,32,0,0]+[32]*7}
    render('FM glyph and segment', ['id="symbol-fm" class="symbol on"', 'Radio band display · FM',
                                   'Radio band: FM', 'id="track">87.50'])
    active['radio']={'frequency_khz':531,'band':'AM','display_band':'MW','preset':False}
    active['symbols']['fm']=False
    active['title']['codes']=[139,140,32,0,0]+[32]*7
    render('MW without clock AM', ['id="symbol-fm" class="symbol off"', 'Radio band display · MW',
                                  'MW RADIO', 'id="symbol-am" class="symbol unmapped '])
    active['radio']['display_band']='LW'
    active['radio']['frequency_khz']=153
    active['title']['codes']=[138,140,32,0,0]+[32]*7
    render('LW band glyph', ['LW RADIO', 'Radio band display · LW'])
    active['symbols'].update(repeat=True,repeat_one=False)
    render('repeat all', ['id="symbol-repeat" class="symbol on"',
                          'id="symbol-repeat-one" class="symbol off"'])
    active['symbols']['repeat_one']=True
    render('repeat one', ['id="symbol-repeat" class="symbol on"',
                          'id="symbol-repeat-one" class="symbol on"'])
    active['symbols'].update(repeat=False,repeat_one=False)
    render('repeat off', ['id="symbol-repeat" class="symbol off"',
                          'id="symbol-repeat-one" class="symbol off"'])
    active['symbols']['shuffle']=True
    render('shuffle on', ['id="symbol-shuffle" class="symbol on"',
                          'id="symbol-repeat" class="symbol off"'])
    active['symbols']['shuffle']=False
    render('shuffle off', ['id="symbol-shuffle" class="symbol off"'])
    active['symbols']['pgm']=True
    render('PGM lit phase', ['id="symbol-pgm" class="symbol on"'])
    active['symbols']['pgm']=False
    render('PGM dark phase', ['id="symbol-pgm" class="symbol off"'])
    active['symbols'].update(step=True, track=True, pgm=True)
    render('STEP TRACK with PGM', ['id="symbol-step" class="symbol on"',
                                  'id="symbol-track" class="symbol on"',
                                  'id="symbol-pgm" class="symbol on"'])
    active['symbols']['pgm']=False
    render('STEP TRACK steady through PGM dark', ['id="symbol-step" class="symbol on"',
                                                 'id="symbol-track" class="symbol on"',
                                                 'id="symbol-pgm" class="symbol off"'])
    active['symbols'].update(step=False,track=False)
    render('STEP TRACK off', ['id="symbol-step" class="symbol off"',
                              'id="symbol-track" class="symbol off"'])
    active = copy.deepcopy(base)
    active['source']='md'
    active['numeric']=None
    active['program']={'selected_track':1,'track_blank':False}
    render('program track one', ['MD PROGRAM', 'Program track', 'id="track">001',
                                 'id="elapsed-group" hidden=""'])
    active['program']['selected_track']=20
    render('program track twenty', ['id="track">020'])
    active['program']={'selected_track':None,'track_blank':True}
    render('program selection blank phase', ['MD PROGRAM', 'Program track'], ['id="track">020', 'Check numeric data'])
    active = copy.deepcopy(base)
    render('exit program to playback', ['id="track">20', 'id="time">00:14'], ['MD PROGRAM'])
    active = copy.deepcopy(base)
    active['message']={'mode':1,'code':132}
    active['title']={'text':'Program On','codes':list(b'Program On')+[255]*2}
    render('Program On message', ['Title: Program On', 'Player message'], ['Display transition'])
    active['message']['code']=133
    active['title']={'text':'Program Off','codes':list(b'Program Off')+[255]}
    render('Program Off message', ['Title: Program Off', 'Player message'], ['Display transition'])
    active['message']['code']=30
    active['title']={'text':'Blank Disc','codes':list(b'Blank Disc')+[255]*2}
    render('Blank Disc message', ['Title: Blank Disc', 'Player message'], ['Display transition'])
    active['message']['code']=5
    active['title']={'text':'No Disc','codes':list(b'No Disc')+[255]*5}
    render('No Disc message', ['Title: No Disc', 'Player message'], ['Display transition'])
    active = copy.deepcopy(base)
    render('ordinary title after message', ['12-character display window'], ['Player message'])
    active = copy.deepcopy(base)
    active['source']='md'
    active['numeric']=None
    active['track_display']={'number':1,'indicators':1048575,'playing':False}
    active['symbols']={'over20':True,'over20_inferred':True}
    render('stopped track with blank time', ['id="track">01', 'id="elapsed-group" hidden=""'],
           ['class="numbers stale"'])
    active['track_display']={'number':21,'indicators':0,'playing':True}
    active['numeric']={'track':21,'minutes':0,'seconds':5,'indicators':0}
    render('playing track21 with time', ['id="track">21', 'id="time">00:05',
                                        'id="symbol-over20" class="symbol on candidate"'],
           ['id="elapsed-group" hidden=""'])
    active['track_display']['playing']=False
    active['numeric']['minutes']=56
    active['numeric']['seconds']=24
    render('stopped disc summary time', ['id="track">21','id="time">56:24',
                                         'id="elapsed-group-label" class="eyebrow">Time'])
    active = copy.deepcopy(base)
    active['symbols']={'rec':True}
    render('main REC on', ['id="symbol-rec" class="symbol boxed rec on"'])
    active['symbols']['rec']=False
    render('main REC off', ['id="symbol-rec" class="symbol boxed rec off"'])
    active = copy.deepcopy(base)
    active.update(source='radio', mode='radio', radio=None, numeric=None, track_display=None,
                  radio_recording={'track':2,'minutes':70,'seconds':19,'indicators':3},
                  symbols={'radio':True,'rec':True,'record_date':True,'remain':True,'remain_inferred':True})
    render('radio recording track 2 lit', ['id="track">02', 'id="time">70:19',
           'id="elapsed-group-label" class="eyebrow">Remaining',
           'id="symbol-record-date" class="symbol on"', 'id="symbol-remain" class="symbol on candidate"',
           'aria-label="Track 1, indicator on"', 'aria-label="Track 2, indicator on"'],
           ['id="elapsed-group" hidden=""', 'unmapped in radio mode'])
    active['radio_recording']['indicators']=1
    render('radio recording track 2 blank phase', ['aria-label="Track 1, indicator on"',
           'aria-label="Track 2, indicator off"', 'id="time">70:19'])
    offline = True
    render('device disconnected', ['Device disconnected', 'class="numbers stale"'],
                                 ['class="status live"'])
finally:
    server.shutdown()
