# Title decoding: corrected sampling

The constant-title disc experiment established a correction to the earlier
falling-edge interpretation. Use **rising-edge samples, LSB-first bytes**
(CPOL=1, CPHA=1 / mode 3 with the observed idle-high clock).
The old bit-reversed seven-bit SDO2 rule recovered stable text by sampling
around a transition; it is superseded by the direct byte encoding below.

SDI2 bytes 2-13, zero-based, contain twelve character cells as ordinary ASCII.
Unused suffix cells are 0xff. Control values 0 and 1 occur during transitions;
their exact rendering is unknown. The two SPI-2 data lines usually agree during
settled text but can differ during updates; use SDI2 for the input text stream.
Physical driver ownership is not established solely from that agreement.

All 62 alphanumeric characters and the punctuation `!?+-_./:;` are recovered
from the retained title-test CSVs. Space is independently observed in TRACK 10.
Untested character ranges remain unvalidated. The replay displays the current
window, not a reconstructed full scrolling title.

Reproduce with `python3 analysis/analyze_titles.py`; output is retained in
`title-evidence.txt`. The script compares the same direct-byte interpretation
across all three streams. Tests require the final windows in all twelve files.

Capture labels still require care: file 10 contains PLAY, file 11 contains
TRACK 10, and file 12 begins with 0123456789. With corrected SDI2 sampling,
file 12 ends with nine spaces and LON entering the window. It does not contain
a full scroll of the long title. The sixth sample contains eleven characters,
not twelve, followed by 0xff padding.

See `test20-findings.md` for track number, time, and the 20 indicator bits.
