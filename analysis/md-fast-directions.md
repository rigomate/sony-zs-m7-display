# Fast MD disc animation direction

User-labelled captures:

- `captures/manual2/playback-disc-icon-spinning-backwards-fast.sal`
- `captures/manual2/playback-disc-icon-spinning-forward-fast.sal`

The user identified the target as the boxed MiniDisc graphic. Both Saleae files
were exported to the sibling `*-raw` folders. They contain51 and48 complete
numeric frames respectively. In both, source byte3 low nibble is MD, byte1 is
`0x20` (play), and the disc graphic field byte2 is constant within each capture:
`0x02` for the user-labelled backward fast sweep and `0x03` for the forward
fast sweep. The compared payloads otherwise show the same transport layout;
the time digits change as playback moves. This isolates byte2 values2/3 as the
two fast animation direction states in this context.

`decode_md_transport` now accepts byte2=2 and3 when byte1 marks playback. The
live API reports `animated-reverse-fast` or `animated-forward-fast`; the page
uses opposite wedge phase orders and a faster approximate CSS cycle. After the
user viewed the result and reported the apparent directions were swapped, the
two CSS phase orders were reversed; numeric byte-to-capture mapping did not
change. The user then observed normal, forward and reverse all remained
visually opposite, so the final CSS update inverted all three phase orders.
Timing is not synchronized to the physical display's phase. Existing byte2=1
normal playback and byte2=7 pause protocol interpretations remain unchanged.

Native captured-frame replay and browser class/animation checks pass. No player
commands were transmitted.
