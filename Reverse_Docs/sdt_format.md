# SDT data tables and the initial handedness screen

## Scope

The Symbian 3rd build uses SDT as an extension for several unrelated data tables, not as proof of one universal container. This document covers the two UI tables parsed by `DialogManager`:

- `str.sdt`: texture and sound resource-name lists;
- `state.sdt`: dialog visuals, states, and animation keys.

The reference data is `Reversed_Game/src/resources/res.spk`, using the Chinese SPK path layout. The UI resources themselves include the English labels seen in the handedness screen.

Recovered parser entry points are `LoadStrSdtResource` (`0x000f4ecc`) / `ParseStrSdtTable` (`0x000f50a0`) and `LoadStateSdtResource` (`0x000f65e0`) / `ParseStateSdtTable` (`0x000f5570`). All multibyte integer fields below are little-endian unless explicitly noted otherwise.

## `str.sdt`

```text
u32 textureNameCount
u32 unknown0
TextureName textureNames[textureNameCount]
u32 soundNameCount
TextureName soundNames[soundNameCount]
```

The reference file has 136 texture names, `unknown0 = 5`, and three sound names. The parser begins the first name at byte offset 8, not byte offset 4.

Each name record occupies exactly 24 bytes:

```text
u16be textLength
char  text[textLength]
u8    padding[22 - textLength]
```

The byte-swapped length is intentional: the ARM parser combines the first two bytes rather than reading a native little-endian `u16`. Text is ASCII and converted to UTF-16 by the original loader. The real resource names are at most 22 bytes.

`DialogManager` interns each texture name and obtains a type-3 `TextureRes` at cache level 1 through `ResManager_GetResource`. The directory is language-dependent (`ui` or `uien`). State-table visual records refer to these resources by their zero-based texture-name index.

## `state.sdt`

### Overall hierarchy

```text
u32 cachedTotal
u32 dialogCount
Dialog dialogs[dialogCount]
```

The reference table has `cachedTotal = 390` and seven dialogs. Its 390 visual records happen to be matched by 390 state elements across 30 states. The parser stores the first word at manager offset `+0x5c`; its precise intended meaning remains unconfirmed.

Each dialog is serialized as:

```text
u32 visualCount
Visual visuals[visualCount]
u32 stateCount
State states[stateCount]
```

A visual record is three words:

```text
u32 resourceNameIndex
s32 initialX
s32 initialY
```

The parser constructs a 0x34-byte runtime visual record for each entry and obtains its texture dimensions. State elements address visuals by their zero-based position in this array, not directly by resource-name index.

A state begins with:

```text
u32 stateId
u32 elementCount
Element elements[elementCount]
```

State selection in the game uses the state's array index. `stateId` is a separate data value and must not be substituted for that index.

### Elements

Every element has a four-word header:

```text
u32 visualIndex
s32 animationKeyCount
u32 textOrBehavior
u32 secondaryTextureSelector
AnimationKey keys[max(animationKeyCount, 0)]
```

`animationKeyCount = -1` is valid and represents an empty entry. It must not be treated as a huge unsigned allocation.

The third word is zero for the image-only handedness elements; nonzero entries take other UI behavior and are not covered by the current image renderer. The fourth word participates in secondary-texture setup. Both names are provisional outside the confirmed image path.

### Animation keys

Each key is 13 signed 32-bit words (52 bytes):

| Word | Recovered image-path meaning |
|---:|---|
| 0 | Time/frame selector. |
| 1, 2 | Destination left and top. |
| 3, 4 | Destination width/height numerator factors. |
| 5 | Screen-blitter mode. |
| 6, 7 | Source left and top. |
| 8, 9 | Source rectangle width and height. |
| 10, 11 | Destination width/height divisors. |
| 12 | Unidentified; `-1` in the initial screen. |

For the ordinary image path, the source rectangle is:

```text
source = [word6, word7, word6 + word8, word7 + word9]
```

and the destination rectangle is:

```text
left   = word1
top    = word2
right  = word1 + word8 * word3 / word10
bottom = word2 + word9 * word4 / word11
```

Coordinates use right/bottom-exclusive rectangles. The original state code can interpolate between keys or hide an element outside its keyed range. The initial screen only requires exact key matches at times zero and one.

`Dialog_SetStateTime` (`0x000fb49c`) stores the time and calls `Dialog_SelectState` (`0x000faff4`). `Dialog_ApplyState` (`0x000f8748`) applies keys, and `Dialog_RenderElement` (`0x000fac78`) sends the resulting rectangles and textures to `ScreenBlitter`. `Dialog_SetElementTime` (`0x000f9258`) contains the interpolation path.

## Initial handedness state

`GameMenu` is the first dialog in the manager. Its initial game-state code calls:

```c
Dialog_SetStateTime(gameMenu, 7, selection);
```

Thus the first screen is **state array index 7**, whose stored `stateId` is 2. It contains five elements:

| Element | Visual | Resource | Key time | Destination |
|---:|---:|---|---:|---|
| 0 | 44 | `black` | 0 and 1 | `(0,0)`, scaled to 320×240 from 1×1. |
| 1 | 45 | `left_shouji_01` | 0 | `(106,63)`, 55×116. |
| 2 | 46 | `Return_shouji_01` | 1 | `(106,63)`, 55×116. |
| 3 | 47 | `left_01` | 0 | `(219,54)`, 13×142. |
| 4 | 48 | `right_01` | 1 | `(219,54)`, 13×142. |

The two phone resources are mirrored variants. `left_01` and `right_01` contain the same combined “Left Hand / Right Hand” bitmap with different palettes; they are not SBN-rendered strings. The selected palette highlights one label.

The internal framebuffer is 320×240. The original `BasicShader_Render` (`0x000aec98`) maps source `(x,y)` to linear destination index:

```text
destinationIndex = height * (width - 1 - x) + y
                 = 240 * (319 - x) + y
```

This produces the 240×320 portrait image in which the phone and labels appear horizontal. The portable SDL backend presents that portrait result directly.

Acceptance stores control orientation 1 or 2 through `0x000719ac`, then advances into logo/loading states. Those later states and full dialog text behavior remain reconstruction work.

## Portable implementation and verification

`Reversed_Game/src/soft3d/UiManager.*` implements both parsers and the exact-key image path under a portable functional name. RTTI identifies the original class as `DialogManager`, with `Renderable` as the primary base and `s3dBase` as a secondary base. Its virtual order is `Render`, `Update(context)`, then `SetEngine`; `Render` calls `Update(0)`, and the secondary `s3dBase` vtable uses an adjustment thunk for `SetEngine`. The portable class now retains that interface and dispatch structure. `ScreenBlitter.*` handles palettized STX sources, transparent indices, clipping, and destination scaling. `GameEngine` loads the real tables during initialization, and the SDL application displays the initial state and accepts direction input.

`tests/test_ui.cpp` validates the complete real archive parse, seven-dialog hierarchy, handedness resources and state, presented frame size, and deterministic switching between left and right selections. `tests/test_engine.cpp` includes a non-square 3×2 presentation mapping check so the transposed traversal cannot be accidentally reversed.
