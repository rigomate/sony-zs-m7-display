# Constant-title 20-track disc measurement

User supplied a newly inserted 20-track MiniDisc; all track titles are `TEST`.
Screenshot shows track durations including 1=5:59, 2=4:22, 3=4:19,
4=6:49, 5=6:13, 6=6:20, 7=6:12, 8=5:15, 9=2:15,
10=1:34, 11=1:34. The screenshot is from the preparation device and does
not establish the ZS-M7's current track.

Capture uses Saleae Logic DAAD127308D7F0CD, channels 0-7 at 8 MS/s.
Wiring remains as specified in HANDOFF.md. Only `play` and `track_next`
are transmitted; no player stop/power command is sent.

`run.jsonl` records wall-clock timestamps, capture IDs, adapter ACKs,
and artifact save/export responses. ACKs establish transmission only.
Numbers in filenames are intended sequence positions, not verified track IDs.
Initial capture records 140 seconds, starting before `play`, for elapsed-time
analysis. Subsequent captures record 16 seconds starting before each next
command, including a minimum three-second delay after its acknowledgment.

The adapter and Saleae MCP are accessible through the host executor. The
sandbox's device namespace differs; absence of ttyUSB0 inside that namespace
does not imply that the physical adapter is disconnected.

Completed: all twenty .sal files and twenty raw CSVs saved and exported.
Final capture independently decodes track 20, TEST, and 0:14 near capture end;
its indicator mask is 0x80000. Playback was left running.
All three CTest suites passed, including every settled frame across this run.
See ../../analysis/test20-findings.md and ../../analysis/test20-validation.txt.
