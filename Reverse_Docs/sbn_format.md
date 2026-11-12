# SBN font and string format

## Scope

SBN stores bitmap glyphs plus coded text strings. This document covers the Symbian 3rd (S60v3) **32-bit encoding**: `basefont.sbn` with the shared base glyphs, and `system.sbn` with per-language local glyphs and the string table. The GA330 build uses the same 32-bit layout; the Win32 build uses a different 7-bit encoding parsed by `Tools/sbn.py` (`-m win32`) and is not covered here.

All multibyte integer fields are little-endian. The reference data is the Chinese `Reversed_Game/src/resources/res.spk` archive.

## File layout

Base font (`basefont.sbn`):

```text
u32 maxHeight
u32 baseGlyphCount
Glyph baseGlyphs[baseGlyphCount]
```

Additional local font and strings (`system.sbn`):

```text
u32 localGlyphCount
Glyph localGlyphs[localGlyphCount]
u32 stringCount
String strings[stringCount]
```

Glyph:

```text
u32 width
u32 height
u32 dataByteCount
u8  data[dataByteCount]
```

String:

```text
u32 codeCount
u32 codes[codeCount]
```

The reference Chinese archive has 64 base glyphs in `basefont.sbn` (1764 bytes), and 18 local glyphs plus 581 strings in `system.sbn`. After each glyph, the original cursor advances by `dataByteCount & ~3`, not by a rounded-up size. All glyph sizes in these files are already multiples of four, so the two rules agree on the observed data.

For languages other than 2, `ParseBaseFontData` adds 2 to `maxHeight`.

## Character codes

Codes below 3 have zero metrics. Code 3 is blank: its height comes from base glyph 0, but its width comes from base glyph 3 (the record at base + 0x24), not from glyph 0. Codes 4 and above select base glyph `code - 4`, then local glyph `code - 4 - baseGlyphCount`.

`VisString_AppendText` (`0x000de3f8`) maps characters to codes as follows:

| Input | Code |
|---|---|
| `a`-`z` | 4-29 |
| `A`-`Z` | 30-55 |
| `0`-`9` | 56-65 |
| `-` | 66 |
| below `-` | blank (code 3) |
| `.` and above | 67 |

`VisString` control codes in string data:

| Code | Meaning |
|---:|---|
| 0 | Newline. |
| 1 | Set color; followed by a `0x00RRGGBB` word. |
| 2 | Restore color. |
| 3 | Blank/space. |

The punctuation tail observed by the older parser notes (and `Tools/sbn.py`) is `67 = …`, `69 = ?`, `70 = '`, `71 = !`; other unassigned codes render as blank. The ARM mapping above is simpler than that table: it sends every character from `.` upward to code 67.

## Recovered parser and loader entry points

| Address | Symbol | Recovered behavior |
|---:|---|---|
| `0x000dd370` | `ParseSbnFontData` | Parses `system.sbn` local glyphs and string table. |
| `0x000dd5ac` | `ParseBaseFontData` | Parses `basefont.sbn`; adds 2 to `maxHeight` for languages other than 2. |
| `0x000defdc` | `LoadBaseFontResource` | Loads the base font resource. |
| `0x000df514` | `LoadSbnFontResource` | Loads an SBN font/string resource. |
| `0x000de3f8` | `VisString_AppendText` | Maps input characters to glyph codes per the table above. |
| `0x000de72c` | `VisString_AppendNumber` | Appends decimal digit codes. |
| `0x000e11a4` | `StringManager_DrawChar` | Draws one glyph into the framebuffer. |

## Glyph drawing

`StringManager_DrawChar` treats glyph data as packed row-major bits, least-significant bit first within each byte. A set bit writes `color | 0x7fff0000` into the 32-bit RGB565/depth framebuffer, with clipping against the configured target. Point anchors use bit 1 for right, bit 2 for horizontal center, bit 5 for bottom, and bit 6 for vertical center.

## Soft2D SDK correlation

The S2D `Font` accessors use the same 12-byte glyph records (width at +0, height at +4, data at +8) and the same character-code mapping. Soft3D adds a `0x14`-byte `s3dBase`/`s3dIO` prefix to the `Font` and `StringManager` layouts. The constructor at `0x000de9ec` is `StringManager_Ctor`, not `BaseFont_ctor`; `Font_Ctor` is at `0x000dc7f8`. See `s2dsdk_correlation.md` for offsets and accessor addresses.

The initial handedness labels are STX palette images, not SBN text. See `sdt_format.md` for their state-table records.

## Portable implementation and verification

`Reversed_Game/src/soft3d/Font.*` implements the recovered parsing and drawing, and `tests/test_font.cpp` validates it against both real SBN resources from the reference archive.
