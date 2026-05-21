# FM Song Effect Reference

Effects are written as 4-character cells after note, instrument, and volume.

```text
C-3007F....
E-3....EA01
G-3....0308
C-4....0300
```

`eggsfm` implements these core and OPN/YM2612-specific effects in the
event-driven song path.

| Effect | Name | Status | Scope | How it Stops | Notes |
|---|---|---|---|---|---|
| `01xx` | Pitch slide up | Implemented | Continuous | `0100`, `0200`, `030x`, or `OFF` | Raises pitch until stopped. Useful for rises and SFX. Larger `xx` is faster. |
| `02xx` | Pitch slide down | Implemented | Continuous | `0200`, `0100`, `030x`, or `OFF` | Lowers pitch until stopped. Useful for falls and SFX. Larger `xx` is faster. |
| `03xx` | Portamento to note | Implemented | Continuous until target or changed | `0300`; reaching target also ends the active slide | New note becomes the target; current channel slides toward it without retriggering. Larger `xx` is faster. |
| `04xy` | Vibrato | Implemented | Continuous | `0400` or `OFF` | Periodically bends pitch around the current/base pitch. `x` is speed, `y` is depth. Speed `0` is slowest; depth `0` turns it off. |
| `07xy` | Tremolo | Implemented | Continuous | `0700` or `OFF` | Periodically modulates volume by updating carrier TL. `x` is speed, `y` is depth. Speed `0` is slowest; depth `0` turns it off. |
| `0Axy` | Volume slide | Implemented | Continuous | `0A00` or `OFF` | Slides volume up/down. `x` is up amount and `y` is down amount. |
| `E1xy` | Note slide up | Implemented | One-shot/targeted slide | Ends after requested semitone distance | Slides up by `y` semitones at speed `x`. |
| `E2xy` | Note slide down | Implemented | One-shot/targeted slide | Ends after requested semitone distance | Slides down by `y` semitones at speed `x`. |
| `E5xx` | Fine pitch | Implemented | Persistent channel setting | Reset with `E580` | Applies a fixed fine pitch offset. `80` is neutral. |
| `EAxx` | Legato toggle | Implemented | Persistent channel mode | `EA00` | `EA01` or any nonzero value turns legato on. While on, new notes change pitch without key-off/key-on. |
| `F5xx` | Disable macro | Implemented | Persistent channel mask | `F6xx` | Furnace-compatible macro disable shape. `00` disables all macros; otherwise `xx` is an eggsfm macro target id. |
| `F6xx` | Enable macro | Implemented | Persistent channel mask | `F5xx` | `00` enables all macros; otherwise `xx` enables and restarts that target macro for the current patch. |

## OPN/YM2612 Effects

These are Furnace-compatible OPN2 effects. They mutate a per-channel live patch,
so the original instrument remains unchanged and later volume/tremolo updates
continue to stack correctly.

| Effect | Name | Status | Scope | How it Stops | Notes |
|---|---|---|---|---|---|
| `10xy` | OPN LFO params | Implemented | Global chip setting | `1000` | `x` enables/disables LFO, `y` is LFO speed 0-7. |
| `11xx` | Feedback | Implemented | Persistent live channel patch | Next instrument or another `11xx` | Sets channel feedback 0-7. |
| `12xx` | Operator 1 TL | Implemented | Persistent live channel patch | Next instrument or another `12xx` | Sets operator total level 0-127. Higher is quieter. |
| `13xx` | Operator 2 TL | Implemented | Persistent live channel patch | Next instrument or another `13xx` | Sets operator total level 0-127. |
| `14xx` | Operator 3 TL | Implemented | Persistent live channel patch | Next instrument or another `14xx` | Sets operator total level 0-127. |
| `15xx` | Operator 4 TL | Implemented | Persistent live channel patch | Next instrument or another `15xx` | Sets operator total level 0-127. |
| `16xy` | Operator multiplier | Implemented | Persistent live channel patch | Next instrument or another `16xy` | `x` is operator 1-4, `y` is multiplier 0-15. |
| `19xx` | All operators attack | Implemented | Persistent live channel patch | Next instrument or AR effect | Sets AR 0-31 on all operators. |
| `1Axx` | Operator 1 attack | Implemented | Persistent live channel patch | Next instrument or another AR effect | Sets AR 0-31. |
| `1Bxx` | Operator 2 attack | Implemented | Persistent live channel patch | Next instrument or another AR effect | Sets AR 0-31. |
| `1Cxx` | Operator 3 attack | Implemented | Persistent live channel patch | Next instrument or another AR effect | Sets AR 0-31. |
| `1Dxx` | Operator 4 attack | Implemented | Persistent live channel patch | Next instrument or another AR effect | Sets AR 0-31. |
| `30xx` | Envelope hard reset | Implemented | Persistent channel mode | `3000` | Nonzero uses hard mute before retriggering a pending note. |
| `50xy` | Operator AM enable | Implemented | Persistent live channel patch | Next instrument or another `50xy` | `x` is operator 1-4, `0` means all. `y` nonzero enables AM. |
| `51xy` | Operator sustain level | Implemented | Persistent live channel patch | Next instrument or another `51xy` | `x` is operator 1-4, `0` means all. `y` is SL 0-15. |
| `52xy` | Operator release rate | Implemented | Persistent live channel patch | Next instrument or another `52xy` | `x` is operator 1-4, `0` means all. `y` is RR 0-15. |
| `53xy` | Operator detune | Implemented | Persistent live channel patch | Next instrument or another `53xy` | `x` is operator 1-4, `0` means all. Furnace detune values `0..7` map to OPN DT. |
| `54xy` | Operator rate scale | Implemented | Persistent live channel patch | Next instrument or another `54xy` | `x` is operator 1-4, `0` means all. `y` is RS 0-3. |
| `55xy` | Operator SSG-EG | Implemented | Persistent live channel patch | Next instrument or another `55xy` | `x` is operator 1-4, `0` means all. `y` 0-7 enables SSG-EG shape, 8 disables it. |
| `56xx` | All operators decay rate | Implemented | Persistent live channel patch | Next instrument or DR effect | Sets DR 0-31 on all operators. |
| `57xx` | Operator 1 decay rate | Implemented | Persistent live channel patch | Next instrument or DR effect | Sets DR 0-31. |
| `58xx` | Operator 2 decay rate | Implemented | Persistent live channel patch | Next instrument or DR effect | Sets DR 0-31. |
| `59xx` | Operator 3 decay rate | Implemented | Persistent live channel patch | Next instrument or DR effect | Sets DR 0-31. |
| `5Axx` | Operator 4 decay rate | Implemented | Persistent live channel patch | Next instrument or DR effect | Sets DR 0-31. |
| `5Bxx` | All operators sustain rate | Implemented | Persistent live channel patch | Next instrument or SR effect | Sets D2R/SR 0-31 on all operators. |
| `5Cxx` | Operator 1 sustain rate | Implemented | Persistent live channel patch | Next instrument or SR effect | Sets D2R/SR 0-31. |
| `5Dxx` | Operator 2 sustain rate | Implemented | Persistent live channel patch | Next instrument or SR effect | Sets D2R/SR 0-31. |
| `5Exx` | Operator 3 sustain rate | Implemented | Persistent live channel patch | Next instrument or SR effect | Sets D2R/SR 0-31. |
| `5Fxx` | Operator 4 sustain rate | Implemented | Persistent live channel patch | Next instrument or SR effect | Sets D2R/SR 0-31. |
| `60xy` | Operator mask | Implemented | Persistent live channel patch | `600F` or next instrument | `x=0`: `y` is bitmask OP1=1, OP2=2, OP3=4, OP4=8. `x=1..4`: `y` toggles that operator. |
| `61xx` | Algorithm | Implemented | Persistent live channel patch | Next instrument or another `61xx` | Sets ALG 0-7 and reapplies carrier volume mapping. |
| `62xx` | LFO FM depth | Implemented | Persistent live channel patch | Next instrument or another `62xx` | Sets FMS 0-7. Requires chip LFO from `10xy` to be audible. |
| `63xx` | LFO AM depth | Implemented | Persistent live channel patch | Next instrument or another `63xx` | Sets AMS 0-3. Requires chip LFO and AM-enabled operators to be audible. |

Not implemented yet: YM2612 channel 6 legacy sample mode (`17xx`) and extended
channel 3 mode (`18xx`). Those need broader player architecture because they
change the channel layout rather than just editing a playing FM voice.

## Patch Macros

Patch macros are C++-defined instrument automation sequences. They are parsed
once, attached to a patch target, reset on note-on, and advanced once per song
tick. They do not retrigger the envelope. ARP changes only the frequency
registers.

```cpp
XfmMacro arp = {};
xfm_macro_parse(&arp, XFM_MACRO_ARP, "0 4 7 | 12 7 4");
xfm_macro_set(module, 0, &arp);
xfm_patch_macro_set(module, 0x20, XFM_MACRO_ARP, 0);

XfmMacro tl1 = {};
xfm_macro_parse(&tl1, XFM_MACRO_TL1, "20 24 28*2 | 32");
xfm_macro_set(module, 1, &tl1);
xfm_patch_macro_set(module, 0x20, XFM_MACRO_TL1, 1);
```

Compact macro syntax:

```text
2 2 31 2 | 2 3 4
12*4 10 8 | 6*2
0 4 7 | 12 7 4
```

- Values may be negative. This is useful for `ARP` and `DT`.
- `|` marks the loop start.
- `value*count` expands repeated values.
- Maximum compiled length is 64 values.

Macro target ids:

| ID | C++ target | Meaning |
|---:|---|---|
| `01` | `XFM_MACRO_TL1` | Operator 1 total level |
| `02` | `XFM_MACRO_TL2` | Operator 2 total level |
| `03` | `XFM_MACRO_TL3` | Operator 3 total level |
| `04` | `XFM_MACRO_TL4` | Operator 4 total level |
| `05` | `XFM_MACRO_MUL1` | Operator 1 multiplier |
| `06` | `XFM_MACRO_MUL2` | Operator 2 multiplier |
| `07` | `XFM_MACRO_MUL3` | Operator 3 multiplier |
| `08` | `XFM_MACRO_MUL4` | Operator 4 multiplier |
| `09` | `XFM_MACRO_DT1` | Operator 1 detune, signed -3..3 |
| `0A` | `XFM_MACRO_DT2` | Operator 2 detune, signed -3..3 |
| `0B` | `XFM_MACRO_DT3` | Operator 3 detune, signed -3..3 |
| `0C` | `XFM_MACRO_DT4` | Operator 4 detune, signed -3..3 |
| `0D` | `XFM_MACRO_FB` | Channel feedback |
| `0E` | `XFM_MACRO_ARP` | Semitone offset from the played note |

Example row controls:

```text
F50E   ; disable ARP macro
F60E   ; enable/restart ARP macro
F500   ; disable all macros
F600   ; enable/restart all macros for the current patch
```

## Implemented Now

```text
EA01   ; legato on
EA00   ; legato off
0108   ; pitch slide up / speed 08
0208   ; pitch slide down / speed 08
0100   ; stop pitch slide up/down
0308   ; portamento on / speed 08
0300   ; portamento off
040F   ; vibrato, speed 0, depth F
0400   ; vibrato off
070F   ; tremolo, speed 0, depth F
0700   ; tremolo off
0AF0   ; volume slide up / speed F
0A0F   ; volume slide down / speed F
0A00   ; volume slide off
E142   ; note slide up 2 semitones at speed 4
E242   ; note slide down 2 semitones at speed 4
E590   ; fine pitch slightly sharp
E580   ; fine pitch neutral
F50E   ; disable ARP macro
F60E   ; enable/restart ARP macro
1230   ; set OP1 total level
1612   ; set OP1 multiplier to 2
5001   ; enable AM on all operators
5303   ; set all operators DT to 0
600F   ; enable all operators
6107   ; algorithm 7
1003   ; chip LFO on, speed 3
6204   ; LFO FM depth 4
6302   ; LFO AM depth 2
```

## Behavior Notes

- Continuous effects keep affecting later rows until explicitly stopped, replaced,
  or their target is reached.
- One-shot/targeted effects work toward a specific distance or target and then
  naturally finish.
- Legato is a mode, not a pitch effect. It changes how later note rows trigger.
- Portamento is a pitch effect. It changes how later note rows reach their target.
- For comments in textplayer song files, use `;` or the older `--` syntax.
