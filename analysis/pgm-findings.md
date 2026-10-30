# PGM flashing indicator

User confirmed PGM flashing. Passive8s recording:
captures/manual/pgm-blinking/capture.sal.253 complete numeric and253 complete
text frames are exported in the sibling raw directory. No player command sent.

Numeric byte5 alternates83 (125 frames) and8B (128 frames). Their XOR is08,
identifying bit3 as the received PGM illumination flag. After the initial
partial phase, transitions are approximately0.509965s apart. Both phases are
independently represented in the wire traffic, so no local CSS blink timer is
needed. Normal250ms web polling follows the received phase, subject to HTTP
scheduling/capture losses.

Byte5 bit7(80) stays set throughout program editing and is not the PGM blinking
bit; it remains unassigned pending a visible comparison (e.g. STEP). Numeric
fields/track indicators also change in this program-entry layout. No program
step/count decoder or recording/edit operation is inferred from those changes.

API symbols.pgm and the PGM visual use byte5&08. REPEAT40, repeat-one20 and
SHUFFLE10 remain independent. Native capture replay checks all253 frames,
both PGM phases and other mode bits off. Browser checks cover both visible
phases and existing display behavior. Stale/disconnected data dims the received
state; no autonomous flashing continues after reception stops.

## User-confirmed STEP and TRACK

User then confirmed STEP and TRACK are steadily on with flashing PGM. Passive
pgm-step-track recording yields188 complete numeric frames, all byte5=83/8B.
This establishes labels that were not identified in the first PGM-only report.

STEP corresponds to byte5 bit7(80), present in both program-entry phases and
absent in earlier normal/repeat/shuffle/source-only layouts. TRACK corresponds
to bit0(01), present here and in ordinary track layouts, absent in source-only
and date/clock references. User-labelled positive state plus these earlier
layouts supports both assignments. A dedicated user-confirmed pair that toggles
only one of STEP/TRACK has not yet been recorded.

API symbols.step and symbols.track drive the two labels independently of PGM08.
Native replay checks all188 new frames and both received PGM phases; a cleared-bit
fixture checks off. Browser checks keep both labels on through PGM dark/lit and
verify off styling. Program-step numeric values are not decoded by this change.

Current numeric byte5 visual map:

| Mask | Visual |
|---|---|
|80|STEP|
|40|REPEAT|
|20|1 modifier|
|10|SHUFFLE|
|08|PGM|
|04|Unmapped|
|02|Unmapped (clock/blanking correlation only)|
|01|TRACK label|
