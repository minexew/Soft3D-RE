# SDT data tables, generic dialogs, and ScreenBlitter

## Scope

The Symbian 3rd build uses SDT as an extension for several unrelated data tables, not as proof of one universal container. This document covers the two UI tables parsed by `DialogManager` and their consumers:

- `str.sdt`: texture and sound resource-name lists;
- `state.sdt`: dialog visuals, states, animation keys, behavior flags, and secondary-texture references.

The reference data is `Reversed_Game/src/resources/res.spk`, using the Chinese SPK path layout. The UI resources themselves include the English labels seen in the handedness screen.

Recovered parser entry points are `LoadStrSdtResource` (`0x000f4ecc`) / `ParseStrSdtTable` (`0x000f50a0`) and `LoadStateSdtResource` (`0x000f65e0`) / `ParseStateSdtTable` (`0x000f5570`). All multibyte integer fields below are little-endian unless explicitly noted otherwise.

## `str.sdt`

```text
u32 textureNameCount
u32 reserved0
TextureName textureNames[textureNameCount]
u32 soundNameCount
TextureName soundNames[soundNameCount]
```

The reference file has 136 initial texture names, `reserved0 = 5`, and three sound names: `aaa`, `sound`, and `5stday`. The parser begins the first name at byte offset 8 and never reads the second word afterward. No other consumer of that word exists in this build, so its confirmed role is only **reserved/ignored**; assigning it a version, count, or cache-level meaning would be speculation.

Each name record occupies exactly 24 bytes:

```text
u16be textLength
char  text[textLength]
u8    padding[22 - textLength]
```

The byte-swapped length is intentional: the ARM parser combines the first two bytes rather than reading a native little-endian `u16`. Text is ASCII and converted to UTF-16 by the original loader. The real resource names are at most 22 bytes.

`DialogManager_InternTextureName` (`0x000f6838`) returns an existing index when possible. Otherwise it appends a name and obtains a type-3 `TextureRes` at cache level 1 through `ResManager_GetResource`. The directory is language-dependent (`ui` or `uien`). State-table visual records refer to these resources by their zero-based texture-name index.

## `state.sdt`

### Overall hierarchy and real-data inventory

```text
u32 declaredVisualTotal
u32 dialogCount
Dialog dialogs[dialogCount]
```

The first word is stored at manager offset `+0x5c`. It is not used to size any allocation and has no later consumer in this build. The name `declaredVisualTotal` describes its relationship to the data without claiming an unobserved cache policy. The reference value is 390, equal to both the total number of visual records and the total number of state elements.

The real file contains seven dialogs, 30 states, 390 visuals, 390 elements, and 923 animation keys:

| Dialog | Visuals | States | Elements | Keys |
|---:|---:|---:|---:|---:|
| 0 | 78 | 10 | 78 | 98 |
| 1 | 78 | 4 | 78 | 247 |
| 2 | 15 | 1 | 15 | 29 |
| 3 | 13 | 3 | 13 | 47 |
| 4 | 1 | 1 | 1 | 2 |
| 5 | 51 | 1 | 51 | 97 |
| 6 | 154 | 10 | 154 | 403 |

There are 218 elements with behavior zero and 172 with behavior one. Five elements request secondary textures. Every shipped key uses blit mode zero and `extra = -1`; the other modes and exact-key `extra` values are nevertheless supported by the original code and are covered by synthetic tests.

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

The parser constructs a 0x34-byte runtime visual record and initializes its source rectangle from the texture dimensions. State elements address visuals by their zero-based position in this array, not directly by resource-name index.

A state begins with:

```text
u32 stateId
u32 elementCount
Element elements[elementCount]
```

State selection uses the state's array index. `stateId` is a separate data value and must not be substituted for that index.

### Elements and secondary textures

Every element has a four-word header:

```text
u32 visualIndex
s32 animationKeyCount
u32 behavior
u32 secondaryTextureSelector
AnimationKey keys[max(animationKeyCount, 0)]
```

`animationKeyCount = -1` is valid and represents an empty entry. It must not be treated as a huge unsigned allocation.

The base `Dialog` applies and renders only elements whose behavior word is zero. Nonzero elements are handled by derived dialog classes. This word is **not a string index**. Text is drawn separately through `StringManager` by those derived classes.

A nonzero secondary selector instructs the parser to format `<primary-resource-name>_alp`, intern that texture name, and replace the selector with the resulting resource index. The real file produces two additional resources, bringing the interned total from 136 to 138:

- `fengmian_kuang_alp`, used twice;
- `tongjishuzi_alp`, used three times.

`Dialog_RenderElement` installs the primary texture and optional secondary texture, calls the scaled blitter, clears the secondary texture, and restores blit mode zero. Secondary textures must have the same dimensions as the primary and must not have palette type bits `0xc0`; otherwise the blitter disables them.

### Animation keys

Each key is 13 signed 32-bit words (52 bytes). Ghidra's `SdtAnimationKey` uses these names:

| Word | Field | Meaning |
|---:|---|---|
| 0 | `time` | Time/frame selector. |
| 1, 2 | `x`, `y` | Destination left and top. |
| 3, 4 | `width`, `height` | Destination width/height numerator factors. |
| 5 | `blitMode` | Screen-blitter mode, 0 through 5. |
| 6, 7 | `sourceX`, `sourceY` | Source left and top. |
| 8, 9 | `sourceWidth`, `sourceHeight` | Source rectangle width and height. |
| 10, 11 | `widthDivisor`, `heightDivisor` | Destination width/height divisors. |
| 12 | `extra` | Copied on exact keys; forced to `-1` during interpolation. No rendering consumer has been identified. |

Runtime key and visual fields are read as signed 16-bit values even though serialization uses 32-bit words. The blit mode is sign-extended into a 32-bit runtime field.

For an ordinary image element, the source rectangle is:

```text
source = [sourceX, sourceY, sourceX + sourceWidth, sourceY + sourceHeight]
```

and the destination rectangle is:

```text
left   = x
top    = y
right  = x + sourceWidth * width / widthDivisor
bottom = y + sourceHeight * height / heightDivisor
```

Coordinates use right/bottom-exclusive rectangles and signed division truncating toward zero.

### State selection and interpolation

`Dialog_SetStateTime` (`0x000fb49c`) stores the time and calls `Dialog_SelectState` (`0x000faff4`). `Dialog_SelectState` first calls `Dialog_ApplyState` (`0x000f8748`) for every behavior-zero element, then calls `Dialog_RenderElement` (`0x000fac78`) for those elements. `DialogManager_Update` traverses its circular dialog list and updates only active dialogs.

`Dialog_SetElementTime` (`0x000f9258`) scans keys in serialized order:

- an exact time match copies every field and makes the visual visible;
- a time before the first key hides the visual and returns zero;
- a time after the last key, or an empty key list, hides the visual and returns one;
- between two keys it linearly interpolates `x`, `y`, `width`, `height`, and all four source-rectangle fields, using signed 16-bit intermediate arithmetic and division truncating toward zero.

Between keys, `blitMode`, `widthDivisor`, and `heightDivisor` come from the **previous** key, while `extra` is reset to `-1`. Interpolation does not use the next key's mode or divisors.

`Dialog_RenderElementAtTime` (`0x000fa6a0`) combines element-time selection and rendering. The original also has per-element current-time storage and advance helpers, state end-time/advance helpers, key field accessors, and a digit-image number renderer; their names have been applied in Ghidra.

Stage 13 fills the previously unused generic accessors in the portable runtime. `Dialog_GetElementWidth`/`GetElementHeight` (`0x000f8c70`/`0x000f8d2c`) evaluate width/height at an arbitrary time with the same signed-16-bit interpolation. They return one when the selected visual is hidden, including before the first or after the last key. `Dialog_GetElementBlitMode` (`0x000f8de8`) retains the previous key's mode during interpolation. Mutable width/height and source-height accessors, `Dialog_SetIndex`, and element-current-time access are also covered.

`Dialog_BlitVisualInRect` (`0x000f8248`) and `Dialog_BlitVisualAtPoint` (`0x000f8344`) perform direct scaled and unscaled blits of a selected visual through the manager's existing `ScreenBlitter`, applying the dialog origin and visual blend mode. Empty rectangles and missing resources return false. `Dialog_SetVisualResource` (`0x000fbc2c`) grows the visual array when necessary, interns the requested texture, and resets its rectangle from the actual texture dimensions. The portable implementation adds null/bounds checks; `tests/test_ui.cpp` exercises exact scaled and unscaled output, hidden-element sizes, mutable keys, and runtime times.

## ScreenBlitter

`ScreenBlitter_BlitUnscaled` (`0x000d49c8`, formerly `BlitTexture`) and `ScreenBlitter_BlitScaled` (`0x000d4cb0`) share a 26-entry mode table. `ScreenBlitter_SelectMode` (`0x000d53c0`) chooses:

- entries 0–11: six unscaled modes, opaque then color-key variants;
- entries 12–23: the same six scaled modes, opaque then color-key variants;
- entry 24: unscaled secondary-alpha blending;
- entry 25: scaled secondary-alpha blending.

Primary sources are palettized, and `ScreenBlitter_GetPaletteColor` (`0x000d3e1c`) reads little-endian RGB565 entries. Target pixels retain RGB565 in the low 16 bits and depth/high data in the high 16 bits. Mode zero replaces the high bits with `0x7fff0000`; other modes OR that value into the existing high bits.

| Mode | RGB565 operation | Helpers |
|---:|---|---|
| 0 | Copy source. | `ScreenBlitter_*Copy*` |
| 1 | Per-channel saturating addition. | `ScreenBlitter_AddRgb565` (`0x000d3ea4`) |
| 2 | Per-channel subtraction clamped to zero. | `ScreenBlitter_SubtractRgb565` (`0x000d4044`) |
| 3 | 75% destination + 25% source, using masked shifts. | `ScreenBlitter_Blend75Dst25Src` (`0x000c7d38`) |
| 4 | 50% destination + 50% source, using masked shifts. | `ScreenBlitter_Blend50Rgb565` (`0x000c7e44`) |
| 5 | 25% destination + 75% source, using masked shifts. | `ScreenBlitter_Blend25Dst75Src` (`0x000c7f18`) |

The weighted modes intentionally discard low channel bits before shifting; they are not floating-point or rounded blends.

Color-key variants skip pixels whose palette index equals the source transparent index. The key variants exist independently for every mode and for scaled/unscaled drawing.

### Clipping and scaling

Unscaled blits advance source left/top when destination x/y is negative, then clip source right/bottom against the target. Scaled blits compute 16.16 source/destination ratios and clip in this order:

1. destination right against target width, reducing source right;
2. negative destination left, advancing source left;
3. destination bottom against target height, reducing source bottom;
4. negative destination top, advancing source top.

Each scaled row starts with fixed x equal to zero. Fixed y starts at the clipped source top in 16.16 form. Sampling uses `fixedX >> 16` and `fixedY >> 16` with nearest-neighbor truncation. The portable implementation adds source-bound guards for malformed rectangles; valid original input follows the same arithmetic.

`ScreenBlitter_Clear` (`0x000d48f8`) writes RGB565 with zero high bits from the last pixel down through index 1. Its original loop leaves **pixel 0 unchanged**, and the portable implementation preserves that boundary behavior.

### Secondary alpha textures

`ScreenBlitter_BlendSecondaryAlpha` (`0x000c80bc`) applies the secondary byte as a 0–32 alpha value:

- 0 or 1: keep the destination color;
- 2 through 30: interpolate 5/6/5 channels as `destination + (source - destination) * alpha / 32`;
- 31 or greater: copy source RGB565.

The ARM helper packs the three channels into one 32-bit value and performs the interpolation with wrapping 32-bit subtraction/multiplication (`rsb`, `mul`, and `add`). The intermediate subtraction can wrap, but the final `destination * 32 + (source - destination) * alpha` is an in-range weighted sum, so that wrap cancels. Extraction uses red `>> 15` at `0x000c8274`, green `>> 10` at `0x000c8280`, and blue `>> 5` at `0x000c8290`. The portable implementation formerly used red `>> 23`, incorrectly combining the decompiler's separate high-byte store with the full-color expression. With the shipped logo/alpha resources and destination `0x18c4`, black source, and alpha 2, the correct ARM and portable result is `0x10a3`, darkening the backdrop; the former `0x00b3` blue fringe was a portable extraction bug, not original behavior. `tests/test_ui.cpp` covers this real resource sample and exact result.

The unscaled secondary loop only touches pixels with alpha greater than one. The scaled loop ORs the destination high bits before applying the blend even when alpha is zero or one. It also contains an original bug: it compares the integer fixed-x offset with the transparent color instead of comparing the sampled source index. The portable code preserves that bug and covers it with an exact-pixel test.

## StringManager text integration

Text is not encoded in the element behavior word. Derived dialogs query key rectangles and separately invoke `StringManager`. In addition to the existing point/anchor drawing, the recovered generic APIs are:

- `StringManager_DrawStringRangeInRectByIndex` (`0x000df828`);
- `StringManager_DrawStringAtPointByIndex` (`0x000dffcc`);
- `StringManager_DrawStringInRectByIndex` (`0x000e0058`);
- `StringManager_DrawStringInRectWithAnchorByIndex` (`0x000e00dc`);
- `StringManager_DrawStringInRect` (`0x000e0804`);
- `StringManager_DrawStringAtPoint` (`0x000e0ed4`).

The portable `Font` now implements `CountCodesOnWidth`, `CountTextWidthRange`, and line counting. `StringManager` implements unanchored and horizontally anchored rectangle drawing, ranged drawing, color control codes 1/2, line-break code 0, and space code 3. Horizontal anchor bits 2 and 4 select right and center alignment; vertical rectangle-anchor bits are not applied by the original helper. Language-2 text uses word-aware line measurement, while other languages use the simpler character loop. Range drawing uses inclusive `endLine - startLine + 1` accounting for language 2 and ignores the end-line counter in the other-language branch, matching the original control flow.

## Initial handedness state

`MainUIDlg` is the first dialog in the manager. Its initial game-state code calls:

```c
Dialog_SetStateTime(this, 7, selection);
```

Thus the first screen is **state array index 7**, whose stored `stateId` is 2. It contains five behavior-zero elements:

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

This produces the 240×320 portrait image in which the phone and labels appear horizontal. The SDL app's default automatic display presents this initial dialog directly in portrait with unrotated directional input. After acceptance it reconfigures to landscape, reverses the active orientation's shader mapping before upload, and applies the matching SDL input compensation. Explicit `--orientation portrait` and `--orientation landscape` modes force one presentation throughout.

The original renders the current selection before reading directional clicks, so a changed palette appears on the following frame. Direction actions use click, not merely down. Acceptance stores control orientation 1 or 2 through `GameEngine_SetControlOrientation` (`0x000719ac`), then advances into the logo/loading states documented below.

## Startup modes and the sound-enable prompt

RTTI identifies the first dialog as `MainUIDlg`, not `GameMenu`; `GameMenu` was a convenient but incorrect name used by earlier notes. `MainUIDlg_Ctor` is `0x0004e9c0`, `MainUIDlg_Init` is `0x0004eca0`, and `MainUIDlg_Update` is `0x0004f0cc`.

After handedness, `MainUIDlg` uses these modes:

| Mode | Behavior |
|---:|---|
| 1 | Publisher logo in state 9; leaves after animation completion or more than 12 frames. |
| 100 | Soft3D logo in state 0, with manual advancement of behavior-one elements 8 and 9. |
| 2 | Disclaimer in state 1, then loads three save-slot headers. |
| 3 | Sound-enable prompt, described below. |
| 4 | Authorization/registration flow. Its platform services remain outside the portable engine boundary. |
| 5 | Seven-entry main menu in state 5. |
| 6 | Exit confirmation; acceptance saves `config.sdt` and exits, cancel returns to mode 5. |
| 7 | Extra notice in state 1 with manual element 3. |
| 8 | New-game weapon/outfit selection, gated by the three final config flags. |

Mode 3 is the previously undocumented screen shown immediately before the main menu. It is not a separate dialog class: it is `MainUIDlg` state 3 with six elements. The base-rendered resources are `beijing_01`, `activate`, `yes`, `no`, and `tishi_a`; `tishi_b` is behavior one.

Left/up selects **Yes**, while right/down selects **No**. Each change plays UI sound index 1. Acceptance with Accept, Number6, or either soft key:

1. stops music and all sounds;
2. sets global volume to 0 for No or 255 for Yes;
3. resets the main-menu selection and advances directly to mode 5;
4. loads the menu BSP/SPL backdrop if it is not active;
5. loops `e_wind01`.

The prompt's state selection is 1 for No and 0 for Yes. As in handedness, the current visual is drawn before processing that frame's direction click.

## Concrete dialog mapping and behavior

Construction order and SDT indices match exactly. `ParseStateSdtTable` populates objects already registered with `DialogManager`; it does not create replacements.

| Index | RTTI class | Important entries |
|---:|---|---|
| 0 | `MainUIDlg` | Ctor `0x0004e9c0`, update `0x0004f0cc`. |
| 1 | `OptionDlg` | Ctor `0x00049c6c`, update `0x00049ec0`, controls `0x0004a760`. |
| 2 | `AboutDlg` | Ctor `0x0004b5c0`, update `0x0004bbc8`, model preparation `0x0004b78c`. |
| 3 | `HelpDlg` | Ctor `0x0004d460`, update `0x0004d650`, credits `0x0004dd84`, input `0x0004dfd0`. |
| 4 | `PlayDlg` | Ctor `0x0004e34c`, update `0x0004e65c`, input `0x0004e5a8`. |
| 5 | `LoadDlg` | Ctor `0x0004ad4c`, update `0x0004af0c`, play-time drawing `0x0004b2b8`. |
| 6 | `GameUIDlg` | Ctor `0x00055130`, initialization `0x00055360`, update `0x000566cc`. |

`Dialog_UpdateAndRender` (`0x000f8480`) invokes the derived vtable `+0x14` render/state handler first and vtable `+0x10` input/update handler second. `DialogManager_DispatchInput` (`0x000f6c44`) walks active dialogs in reverse list order and stops at the first handler returning nonzero. Non-game `GameEngine_PreUpdate` dispatches the first clicked key in this priority order: Up, Left, Down, Right, soft left, soft right, Accept, then Number1 through Number0.

- `OptionDlg` has main, control-binding, handedness, and confirmation pages. Volume uses the highest set bit of `masterVolume + 1`, clamped to levels 0–8, and writes `1 << level`. Vibration toggles on either horizontal action. Binding changes skip keys already assigned to another action.
- `AboutDlg` is the extras viewer. It browses unlocked models with 16-frame transitions and fixed-point zoom, and browses documents/photos in one circular sequence: documents ascend/descend first, then photos. Document text uses the recovered StringManager range APIs; photos use `photo_200%d` for 1–9 and `photo_20%d` afterward.
- `HelpDlg` draws 140 credit strings at indices 450–589, scrolling two pixels per frame. Ordinary credits loop after the animation; they do not automatically open statistics. Statistics use SDT state 2 and the generic digit renderer for five counters and hours/minutes/seconds.
- `PlayDlg` advances state 0 every frame and deactivates at completion. Accept or Number6 skips directly to new-game startup.
- `LoadDlg` shows three slots and renders empty records or day/play-time information. Acceptance first checks `slot%d.sav`; failure plays UI sound 6, while success enters the game load path.
- `GameUIDlg` owns the large in-game table. Its menu modes are 0/11 transitions, 1 pause, 2 inventory, 3 map, 4 files, 5 options, 6 examine, 7 controls, 8 return-to-menu confirmation, 9 exit confirmation, 10 item action, 12 game over, and 14 combination. Initialization acquires the prompt/QTE/story/door texture set. Episode map/HUD drawing, model submission, and item effects remain host operations rather than invented portable episode logic.

## `config.sdt`

`config.sdt` is an external file opened through `s3dIO`, not an SPK entry and not a `state.sdt` hierarchy. `GameEngine_LoadConfigSdt` is `0x0006f39c`; `GameEngine_SaveConfigSdt` is `0x0006f6a4`. The fixed little-endian layout is 1,252 bytes:

| Offset | Size | Meaning |
|---:|---:|---|
| `0x000` | 32 | Eight secondary-control binding words. |
| `0x020` | 4 | Master-volume word. |
| `0x024` | 4 | Vibration-enabled word. |
| `0x028` | 400 | One hundred unlocked-model words. |
| `0x1b8` | 400 | One hundred unlocked-document words. |
| `0x348` | 400 | One hundred unlocked-photo words. |
| `0x4d8` | 4 | Extra weapon 0 unlock. |
| `0x4dc` | 4 | Extra weapon 1 unlock. |
| `0x4e0` | 4 | New-game options/completion flag. |

The original loader reads the entire file without a layout-size guard. The portable parser requires at least 1,252 bytes and ignores trailing data. Saving refreshes the volume from the audio device before writing. The unlock arrays contain persistence flags only; model names, text indices, item definitions, and other episode content do not belong to this file.

The three `str.sdt` sound names are not the menu sound table used by `AudioManager_PlaySoundByIndex`. UI indices 0–7 resolve to `ui_select`, `ui_selectitem`, `ui_confirm`, `ui_cancel`, `ui_item`, `ui_false`, `ui_paper`, and `t_lock_open`. Stage 1 supplies all 142 entries through `game::GameData`; see `game_data_tables.md`.

Each save slot is a separate 44-byte `slot%d.sav` header. Its confirmed fields are day at `0x00`, play time at `0x14`, and present flag at `0x18`; the other words are consumed by the later game loader and are not reinterpreted here.

## Other SDT families and scope

The extension is also used by unrelated game-data tables. `LoadItemDataSdt` (`0x00045d98`) reads 67 52-byte item records and 65 12-byte text records from `.\common\itemdata.sdt`, exactly matching its 4,272-byte size. `LoadGameItemSdt` (`0x00046288`, formerly `LoadNumberedSdtResourceA`) reads numbered `gameitem` groups with eight-byte headers and 0x60-byte records. `LoadEnemyInfoSdt` (`0x00046750`, formerly `LoadNumberedSdtResourceB`) reads numbered `einfo` groups with nested 0x1c-byte records. These are not variants of the UI hierarchy and must not be parsed with `ParseStateSdtTable`; their complete layouts and portable implementation are in `game_data_tables.md`.

`message.sdt` is a 512-byte license/message blob read into two 256-byte buffers, not a string table. `domain.sdt` is a 36-byte opaque service record consumed by license/network work. `config.sdt` is the separate menu/configuration file documented above.

`CalcRank` at `0x000669fc` is intentionally empty in this build; do not invent a rank algorithm. Full game-data layouts, GameUI/progression gaps, and license consumers are inventoried in `symbian3rd_game.md`. Generic menu configuration and concrete menu dialog flow belong to stage 6 of `engine_completion_plan.md`; their game-specific consumers belong to `game_completion_plan.md`.

## Portable implementation and verification

`Reversed_Game/src/soft3d/UiManager.*` implements both parsers and manager ownership under a portable functional name. RTTI identifies the original manager as `DialogManager`, with `Renderable` as the primary base and `s3dBase` as a secondary base. Its virtual order is `Render`, `Update(context)`, then `SetEngine`; `Render` calls `Update(0)`, and the secondary `s3dBase` vtable uses an adjustment thunk for `SetEngine`.

`Dialog.*` supplies active flags, virtual derived handlers, state selection, exact/interpolated key application, visibility, advancement, key accessors, and digit rendering. `MenuDialogs.*` implements the six menu-family classes; `GameUIDlg.*` implements the in-game dialog and its resource/menu lifecycle. `GameConfig.*` implements exact serialization and orientation-aware controls. `GameEngine` constructs all seven classes before parsing `state.sdt`, dispatches input in the recovered order, loads/saves configuration, and binds the actual UI audio names.

`tests/test_ui.cpp` validates the complete real archive, all 923 exact keys, all 30 state selections, behavior counts, secondary resources, every blitter path, interpolation, clipping, the real logo's exact secondary-alpha result, and handedness click/following-frame behavior. `tests/test_menu.cpp` drives the real startup through the sound prompt and main menu, every concrete dialog family, model/document/photo navigation, GameUI inventory/options/map behavior, UI voices, and a 1,252-byte configuration round trip in a temporary directory. `tests/test_game_data.cpp` adds all shipped item/day tables and their engine integration. All 28 default CTest tests pass; all 22 tests in the SDL-independent build also pass under AddressSanitizer and UndefinedBehaviorSanitizer.
