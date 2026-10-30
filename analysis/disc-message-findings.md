# Blank Disc and No Disc messages

The user supplied the captures `captures/manual2/Blank Disc-on-alphanumeric.sal`
and `captures/manual2/No Disc-on-alphanumeric.sal`, identifying the displayed
text in each. The exported raw traces and decoded frame lists are alongside the
captures.

All 41 complete text frames in the Blank Disc capture have input mode `01`,
message code `1E`, bytes3..6 zero and bytes7..13 spaces. All 48 complete text
frames in the No Disc capture use the same parameter layout with code `05`.
The captured display return line independently contains `Blank Disc` and
`No Disc` followed by spaces. The C3 receives the display input line, so it
reconstructs the messages from the input command; it does not depend on the
return line being connected.

The title decoder recognizes these exact parameter layouts and emits the
confirmed strings while preserving the message code in the API and raw frame in
diagnostics. Other mode01 codes or parameter variants remain unassigned.
