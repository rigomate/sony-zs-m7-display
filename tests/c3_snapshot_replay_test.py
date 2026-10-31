#!/usr/bin/env python3
"""Replay saved C3 API snapshots through the actual native display decoder.

These snapshots sample the most recently received payload on each bus; they
are not synchronized logic-analyzer traces. Incomplete payloads are ignored,
as on the receiver. Assertions require repeated lit and dark phases.
"""
import json
from pathlib import Path
import subprocess
import sys

if len(sys.argv)!=3:
    raise SystemExit('Expected native replay executable and project root')
executable=Path(sys.argv[1])
root=Path(sys.argv[2])

def replay(stem):
    path=root/'captures/manual2'/stem/'state.jsonl'
    lines=[]
    for lineno,line in enumerate(path.read_text().splitlines(),1):
        try:
            sample=json.loads(line)
            buses=sample['state']['buses']
            payloads=[bus['raw_hex'] for bus in buses[:2]]
            timestamp=max(0,round(sample['elapsed']*1000))
        except (KeyError,ValueError,IndexError,TypeError) as exc:
            raise AssertionError(f'{path}:{lineno}: invalid snapshot') from exc
        if len(payloads)!=2 or any('|' in payload or '\n' in payload for payload in payloads):
            raise AssertionError(f'{path}:{lineno}: invalid payload')
        lines.append(f'{timestamp}|{payloads[0]}|{payloads[1]}\n')
    if len(lines)<10:
        raise AssertionError(f'{stem}: capture too short ({len(lines)} snapshots)')
    result=subprocess.run([str(executable)],input=''.join(lines),text=True,capture_output=True,check=True)
    states=[json.loads(line) for line in result.stdout.splitlines()]
    if len(states)!=len(lines):
        raise AssertionError(f'{stem}: expected {len(lines)} results, got {len(states)}')
    return states

def require_phases(stem,field,expected,phase,minimum=2):
    states=replay(stem)
    matches=[s[field] for s in states if isinstance(s.get(field),dict) and
             all(s[field].get(k)==v for k,v in expected.items())]
    lit=sum(item.get(phase) is False for item in matches)
    dark=sum(item.get(phase) is True for item in matches)
    if min(lit,dark)<minimum:
        raise AssertionError(f'{stem}: {field} {expected} phase {phase}: visible={lit} blank={dark}')
    for state in states:
        item=state.get(field)
        if not isinstance(item,dict) or not all(item.get(k)==v for k,v in expected.items()):
            continue
        workflow=state['timer_workflow']
        if field=='clock_setting':
            if workflow['stage']!='unknown':
                raise AssertionError(f'{stem}: clock edit misclassified as timer workflow: {workflow}')
        else:
            stage='volume' if field=='timer_volume_setting' else ('stop' if item['phase']=='off' else 'start')
            mode='rec' if '-rec-' in stem else 'play'
            if workflow['stage']!=stage or workflow['mode']!=mode:
                raise AssertionError(f'{stem}: expected timer {mode}/{stage}, got {workflow}')
            if field=='timer_setting' and workflow['editing']!=item['editing']:
                raise AssertionError(f'{stem}: timer edit focus mismatch: {workflow}')
    print(f'PASS {stem}: {field}, visible={lit}, blank={dark}',flush=True)

def require_source(stem,mode,source):
    states=replay(stem)
    matches=[s['timer_source_selection'] for s in states if isinstance(s.get('timer_source_selection'),dict)
             and s['timer_source_selection'].get('mode')==mode and s['timer_source_selection'].get('source')==source]
    lit=sum(item.get('source_on') is True for item in matches)
    dark=sum(item.get('source_on') is False for item in matches)
    if min(lit,dark)<2:
        raise AssertionError(f'{stem}: {mode}/{source} visible={lit} blank={dark}')
    for state in states:
        item=state.get('timer_source_selection')
        if isinstance(item,dict) and item.get('mode')==mode and item.get('source')==source:
            workflow=state['timer_workflow']
            if workflow['stage']!='select_source' or workflow['mode']!=mode or workflow['source']!=source:
                raise AssertionError(f'{stem}: source state mismatch: {workflow}')
    print(f'PASS {stem}: {mode}/{source}, visible={lit}, blank={dark}',flush=True)

require_phases('live-clock-hour-volume11','clock_setting',{'hour':0,'minute':0,'editing':'hour'},'hour_blank')
clock_states=replay('live-clock-date-and-time')
clocks=[s['clock_display'] for s in clock_states if s.get('clock_display') and
        s['clock_display']['hour']==9 and s['clock_display']['minute']==4 and not s['clock_display']['standby']]
if not any(c['colon_on'] for c in clocks) or not any(not c['colon_on'] for c in clocks):
    raise AssertionError('live-clock-date-and-time: clock or colon phases missing')
if not any(s.get('title') and s['title'].get('text','').strip()=='26y 10m 31d' for s in clock_states):
    raise AssertionError('live-clock-date-and-time: date title missing')
print('PASS live-clock-date-and-time: date and 09:04 clock with colon phases',flush=True)
require_phases('live-timer-play-radio-volume','timer_volume_setting',{'level':20},'blank')
require_phases('live-timer-play-cd-volume','timer_volume_setting',{'level':20},'blank')
require_phases('live-timer-play-md-volume-retained','timer_volume_setting',{'level':15},'blank')
require_phases('live-timer-play-line-volume','timer_volume_setting',{'level':16},'blank')
require_phases('live-timer-play-cd-volume-retained','timer_volume_setting',{'level':16},'blank')
require_phases('live-timer-play-radio-volume-retained','timer_volume_setting',{'level':16},'blank')
require_phases('live-timer-play-radio-start','timer_setting',{'phase':'on','hour':19,'minute':9,'editing':'hour'},'hour_blank')
require_phases('live-timer-play-radio-start-retained','timer_setting',{'phase':'on','hour':8,'minute':50,'editing':'hour'},'hour_blank')
require_phases('live-timer-play-line-start-hour','timer_setting',{'phase':'on','hour':8,'minute':50,'editing':'hour'},'hour_blank')
require_phases('live-timer-play-cd-start-retained','timer_setting',{'phase':'on','hour':8,'minute':50,'editing':'hour'},'hour_blank')
require_phases('live-timer-rec-line-start-hour','timer_setting',{'phase':'on','hour':8,'minute':50,'editing':'hour'},'hour_blank')
require_phases('live-timer-play-cd-setup','timer_setting',{'phase':'on','hour':19,'minute':9,'editing':'hour'},'hour_blank')
# The user advanced quickly: only one retained, visible stop-minute sample.
require_phases('live-timer-play-cd-setup','timer_setting',{'phase':'off','hour':19,'minute':12,'editing':'minute'},'minute_blank',minimum=1)
for stem,source in [('live-timer-play-radio-source','radio'),('live-timer-play-cd-confirmed','cd'),
                    ('live-timer-play-md-confirmed','md'),('live-timer-md-source-glitch','md'),
                    ('live-timer-play-line-source','line')]:
    require_source(stem,'play',source)
require_source('live-timer-rec-setup','rec','radio')
require_source('live-timer-rec-line-source','rec','line')
for stem,phase,hour,minute,field in [
    ('live-timer-rec-start','on',19,9,'hour'),('live-timer-rec-start-minute','on',19,9,'minute'),
    ('live-timer-rec-stop','off',19,12,'hour'),('live-timer-rec-stop-minute','off',19,12,'minute')]:
    require_phases(stem,'timer_setting',{'phase':phase,'hour':hour,'minute':minute,'editing':field},field+'_blank')
require_phases('live-timer-rec-volume','timer_volume_setting',{'level':20},'blank')
require_phases('live-timer-rec-line-volume','timer_volume_setting',{'level':24},'blank')
print('All C3 snapshot replay checks passed',flush=True)
