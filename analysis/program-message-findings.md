# Alternating ON/OFF messages: wording not fully confirmed

User supplied captures/manual/program-on_program_off.sal, initially suggesting
Program Off / Program On. On clarification the user confirmed the ending is
On / Off but could not confirm the prefix. Do not treat the filename as proof
of the literal word Program or assign ON/OFF code order without another reference.

The original recording is unchanged. Raw CSV,130 complete frames per bus and
message-summary.json are in the sibling *-raw directory. SPI-2 byte1 is01 in
all130 frames. Byte2 alternates84 and85, and bytes3–13 are zero throughout.
There are no ASCII bytes spelling either phrase. These are parameterized or
predefined-message commands whose displayed strings are generated downstream.

Observed transitions (seconds, code):
0.021211 85;0.244318 84;1.296108 85;2.379770 84;3.431558 85.
The main message intervals are approximately1.05–1.08s. The recording alone
cannot establish whether this was an automatic alternation or user-driven.

The numeric side stays in program-entry configuration (STEP and REPEAT flags
set, PGM blinking), step field byte10=02, selected-track field byte11=03 orAA
blank. Global program mode does not simply switch off when the message code
changes. No program-enabled boolean is inferred from the words or numeric
PGM bit. Selected-track blinking continues independently of message changes.

An independent passive API watch after the question saw ordinary ASCII TEST
(mode00), so it could not correlate a currently visible ON/OFF phrase to a code.
No remote command was sent and no firmware string mapping was installed.

Next useful reference: a recording containing only one message visually
identified as ending in ON, or a synchronized view associating ON with84/85.
Then identify the prefix from a readable photo before inserting a full string.
The current firmware still renders these payloads as unknown title control
codes; that is a known limitation pending message-ROM mapping.

## Resolved by user-labelled Program On capture

User subsequently supplied program-on.sal and explicitly confirmed it shows
Program On. All23 complete text frames have mode01/code84 with zero parameters.
This resolves84=Program On. In the earlier two-code recording, whose endings
were confirmed ON/OFF,85 is therefore the OFF member. Rendering Program Off
uses that paired-state inference; no separate labelled OFF-only capture exists.
The initial prefix/code-order uncertainty is superseded by this new reference.

Firmware now reconstructs Program On / Program Off only for exact mode01,
code84/85 and zero parameters. Ordinary ASCII bytes and unknown command forms
retain existing decoding. The title window carries the reconstructed string;
the API message object preserves original mode/code, and raw diagnostics retain
the entire packet. No global program-enabled state is inferred from a message.
Program selection and PGM illumination still follow their separate numeric data.

Native replay checks the23-frame ON reference and all130 paired-message frames,
unknown parameter rejection and returning to ASCII. Browser checks cover both
messages and normal-title restoration. No player command was transmitted.
