# Stopped digits, playback time and track21

User requested remote-driven checks of stopped track digits and playback on a
21-track MD, including OVER20. Explicit experiment runner:
analysis/capture_track21.py. Artifacts: captures/track21/* with native Saleae
captures, exported transitions, complete frames and before/after IR snapshots.
Only play, greater_than_10, digit2/digit0/digit1 and disc_name were transmitted.
No STOP, eject, power, recording or settings command was sent.

## Observed remote sequence

-01: current stopped baseline, track digits01 with timeAA/AA.
-02: play acknowledged, triangle flag set and time starts advancing.
-03: >10,2,0 selects20, independently confirmed by byte11=20 and playback.
-04: >10,2,1 selects21, confirmed by byte11=21 and advancing00:00..00:05.
-05: later disc_name display control; the MD was already stopped before that command; the transition was not recorded. Before and after show a stopped summary21 /56:24.

ACK alone is not taken as proof: independent complete bus frames establish the
selected tracks and transport flags. Selection input placeholders FF/2F occur
between digit commands and are not interpreted as BCD track values.

## Numeric fixes

The previous numeric decoder rejected all tracks above20, hiding both track21
and its valid time. It also required valid time to expose any numeric data,
hiding the selected01 in the stopped baseline. These are separate issues.

New track_display exposes the MD track/count field and calendar independently
of time, while program selection remains separate. Playback time now accepts
track21 (valid low-two-digit BCD up to99 with the observed high-position byte12
zero). Higher hundreds encoding remains unverified. Existing numeric time
format remains packed BCD minute/second with seconds<=59.

The UI shows the track/count even when the player sends blank time and hides
that absent time group. When time is present, it renders the received minutes
and seconds, always formatted as four digits MM:SS. The stopped summary labels it Time rather than Elapsed. Calendar
bits remain visible in stop; only an observed play flag marks a current-playing
track. Track21 has no numbered1–20 calendar slot.

## OVER20 inference and limits

The user could not visually confirm OVER20 (too dim). Two initial candidates
were compared against older evidence. Numeric byte9 bit6(40) is also set in the
earlier20-track stopped summary in remote-indicators/01-band/experiment.json
before-state (track20,time64:45). That rules it out as a simple >20-disc flag.

Byte5 bit2(04) is clear in the earlier20-track normal references (byte5=03),
but set in all current21-track stopped/playback frames (05/07). That is the
stronger OVER20 candidate, consistent with the play-mode bit grouping. It remains
set on tracks1,20 and21, so if this mapping is correct it indicates a disc with
tracks beyond20, not only the currently playing21st track.

Firmware exposes this as symbols.over20 plus over20_inferred=true. The visual
has a dashed candidate style and an explicit unconfirmed tooltip. It is not
claimed as a visually isolated physical-bit assignment. A labelled positive
visual or controlled20/21-disc substitution would provide stronger confirmation.

## Validation

Native replay checks stopped01 with blank time, remote-selected20/21, and
advancing time on21. Existing1..20 numeric and program/state regressions are
included. Browser cases verify stopped digits, track21/time, calendar behavior
and the stopped summary. No independent timer or synthetic time advancement is
introduced; values always come from complete captured frames.

## Final live verification

After flashing, the remote was used again: play, >10,2,1. The capture confirms
track21, with seek-time blanking followed by valid playback time. Live API
snapshots then reported elapsed61,62,63seconds and the physical webpage rendered
track21 with01:04. Stopped digits, advancing playback time and the >20 bound
are therefore verified against the actual board, not only fixtures. The player
was left playing21 at the final check. No STOP command was sent; the cause of
the intervening stopped state was not directly observed and is not inferred.
Evidence: captures/track21/flashed-api.jsonl and dashboard.png.
