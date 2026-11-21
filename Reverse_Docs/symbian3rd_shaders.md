# Symbian 3rd presentation shaders

## Scope

This document records the portable presentation effects recovered for Stage 8 from `SymbianBin/out.bin` at image base zero. It supplements `symbian3rd_video.md`: `BasicShader` and `FogShader` were already implemented, while this pass covers `MotionBlurShader`, `GrayShader`, `FadeShader`, `InkShader`, `BlindShader`, and `TwistShader`.

The reconstruction is in `Reversed_Game/src/soft3d/PresentationShaders.*`; directed exact-pixel and state tests are in `Reversed_Game/tests/test_shaders.cpp`. `Renderer::SelectShader` now exposes the original forwarder at `0x00031d8c`.

All source pixels are 32-bit internal framebuffer words: RGB565 in the low halfword and depth or a special marker in the high halfword. Unless stated otherwise, output is a 32-bit `0x00RRGGBB` word. RGB expansion never replicates low channel bits:

```c
expanded = ((pixel & 0xf800) << 8) |
           ((pixel & 0x07e0) << 5) |
           ((pixel & 0x001f) << 3);
```

## Shared orientation traversal

`BasicShader::SetEnabled` maps zero to one; other values are retained. Every 32-bit shader in this document uses orientation one when `enabled == 1` and the alternate traversal otherwise. For source coordinate `(x,y)`, source width `w`, and source height `h`, the destination word index is:

```text
enabled == 1: h * (w - 1 - x) + y
otherwise:    h - 1 - y + h * x
```

The loop order remains source-row-major: outer `y`, inner `x`, with a destination stride of `-h` or `+h`. Ink is the exception because it ignores orientation and works on linear interior neighborhoods. Blind uses the same index formula in **16-bit units**, not 32-bit units.

`Scene_Ctor` (`0x0002a0f4`) owns the effects at Scene offsets `+0x914` Ink, `+0x918` Fade, `+0x91c` Gray, `+0x920` MotionBlur, `+0x924` Blind, and `+0x928` Twist. The effect selector `GameSI_SelectPresentationShader` at `0x000672e4` first completes any active fade, then chooses Gray for value 1, MotionBlur for 2, Blind for 7, Twist for 8, and no custom shader otherwise. MotionBlur selection is followed by `MotionBlurShader_ResetHistory`. `GameSI_FadeOut`/`GameSI_FadeIn` (`0x000656dc`/`0x00065820`) start mode-0 fades while idle and return false until completion, then complete and return true once. Ink is selected by the game-over scene path rather than by command 103. All six owners and command paths are implemented and tested in stage 3; see `symbian3rd_camera.md`.

## MotionBlurShader

### Layout and methods

The original allocation is `0x1c` bytes:

| Offset | Type | Meaning |
|---:|---|---|
| `+0x00` | `BasicShader` | 0x14-byte base. |
| `+0x14` | `uint32_t *` | Owned expanded-color history, `width * height` words. |
| `+0x18` | `int32_t` | Copy-destination-into-history request. |

| Address | Function |
|---:|---|
| `0x000e2710`, `0x000e2680` | `MotionBlurShader_Ctor`, duplicate |
| `0x000e2928` | `MotionBlurShader_ResetHistory` |
| `0x000e2950` | `MotionBlurShader_Render` |
| `0x000e2820`, `0x000e28a4` | Destructor and deleting destructor |

The ARM constructor allocates the history but initializes neither the history contents nor the reset flag. The portable vector starts with defined zero storage and requests a reset, avoiding indeterminate first-frame behavior. Defined original use explicitly calls `ResetHistory` when selecting the effect.

On a reset render, the shader first copies the current destination to history using the active orientation, then clears the request. For each source pixel it expands the RGB565 channels without bit replication and computes, independently with integer truncation:

```text
outputChannel = min(255, currentChannel / 4 + previousChannel * 7 / 8)
```

The result is written to both history and destination. The weights deliberately sum to 10/8, so bright values saturate. Source depth is ignored. Subsequent frames reuse the stored history; calling `ResetHistory` instead seeds it from the destination before blending.

## GrayShader

Gray has no fields beyond `BasicShader`. Its constructor is `0x00030fdc`, render is `0x000e3064`, and destructors are `0x000e4a30`/`0x000e49d8`.

Depth markers `0x6fff` and `0x7fff` bypass the effect and use ordinary RGB565 expansion. Every other pixel uses only its five blue bits:

```text
b = pixel & 0x1f
output = (b << 19) | (b << 11) | (b << 3)
```

This is blue-derived grayscale, not a luma-weighted conversion and not an 8-bit blue expansion followed by replication.

## FadeShader

### Layout and state transitions

The original allocation is `0x34` bytes:

| Offset | Type | Meaning |
|---:|---|---|
| `+0x00` | `BasicShader` | 0x14-byte base. |
| `+0x14` | `Renderer *` | Renderer used to select/deselect the shader. |
| `+0x18` | `int32_t` | Current frame. |
| `+0x1c` | `int32_t` | Frame count/divisor. |
| `+0x20` | `int32_t` | Active flag. |
| `+0x24` | `int32_t` | Completed flag. |
| `+0x28` | `int32_t` | Fade-out selector; zero selects fade-in. |
| `+0x2c` | `uint16_t` | Target RGB565 color. |
| `+0x30` | `int32_t` | Completion mode. |

| Address | Function |
|---:|---|
| `0x000e32f0`, `0x000e3270` | `FadeShader_Ctor`, duplicate |
| `0x000e3370` | `FadeShader_StartFade` |
| `0x000e3414` / `0x000e3448` | `FadeShader_IsCompleted` / `FadeShader_IsActive` |
| `0x000e347c` | `FadeShader_Complete` |
| `0x000e34d0` | `FadeShader_Render` |
| `0x000e4a88`, `0x000e4ae0` | Destructor and deleting destructor |

`StartFade(fadeOut,color,frameCount,mode)` stores the arguments, sets frame and completed to zero, activates the effect, and selects it through the renderer. The original constructor does not initialize the renderer pointer; portable construction accepts it explicitly and guards a null pointer rather than reproducing an invalid call.

An inactive render leaves the destination untouched. Otherwise, if frame differs from frameCount, frame is incremented. If they are already equal, mode zero sets completed while remaining active; nonzero mode calls `Complete`. `Complete` sets completed, clears active, and restores the default shader only for fade-in (`fadeOut == 0`). The current call still draws after this state change. Consequently, reaching the final frame and observing completion require separate render calls.

The blend factor is signed integer division followed by truncation to eight bits at multiplication:

```text
alpha = frame * 31 / frameCount
if fadeOut == 0: alpha = 32 - alpha
blend = alpha & 0xff
```

Thus ordinary nonnegative fades use final factors 31 and 1, not 32 and 0.

### Packed color arithmetic

Depth `0x6fff` alone bypasses fading and copies the expanded source. Unlike Gray, `0x7fff` is faded. Other pixels use packed intermediate values:

```c
sourceColor = ((source & 0xf800) << 10) |
              ((source & 0x07e0) << 5) | (source & 0x1f);
targetColor = ((target & 0xf800) << 10) |
              ((target & 0x07e0) << 5) | (target & 0x1f);
mixed = (targetColor - sourceColor) * blend + sourceColor * 32;
output = ((mixed & 0x7c000000) >> 7) |
         ((mixed & 0x001f8000) >> 5) |
         ((mixed & 0x000003e0) >> 2);
```

All subtraction and multiplication retain the ARM's low 32 bits. This packed operation must not be replaced with independently rounded floating-point channel interpolation. The S2D SDK fade is a different implementation; see `s2dsdk_correlation.md`.

## InkShader

### Layout and engine interaction

The original allocation is `0x3c` bytes. Ink has a secondary `s3dBase` at `+0x14` and an embedded timer at `+0x2c`; neither timer field participates in the render loop.

| Offset | Type | Meaning |
|---:|---|---|
| `+0x00` | `BasicShader` | 0x14-byte base. |
| `+0x14` | `s3dBase` | Secondary engine-linked base. |
| `+0x1c` / `+0x20` | `int32_t` | Active / completed flags. |
| `+0x24` | `Engine *` | Engine receiving hook-state changes. |
| `+0x28` | `int32_t` | Ink channel mode. |
| `+0x2c` | `s3dTimer` | Embedded timer object. |
| `+0x38` | `uint32_t *` | Previous destination/history pointer. |

| Address | Function |
|---:|---|
| `0x000e3908`, `0x000e3830` | `InkShader_Ctor`, duplicate |
| `0x000e39ac` | `InkShader_Reset` |
| `0x000e39e0` | `InkShader_StartInk` |
| `0x000e3a80` | `InkShader_StopInk` |
| `0x000e3ae4` | `InkShader_Render` |
| `0x000e3e58` / `0x000e3e8c` | `InkShader_IsCompleted` / `InkShader_IsActive` |
| `0x000e3ec0` | `InkShader_SetEngine` |
| `0x000e4824` | `InkShader_SubtractInk` |
| `0x000e4b40`, `0x000e4bd0` | Destructor and deleting destructor |

`StartInk` does nothing if engine is null or the effect is already active. It clears completed, stores engine/mode/initial history, disables submit and prepare hooks, and activates. Its third argument is unused. `Reset` clears active and completed. `StopInk` reenables submit and prepare when an engine is present, sets completed, and clears active.

The recovered engine setters are now named `Engine_SetPreUpdateEnabled` (`0x000f2ffc`), `Engine_SetPostUpdateEnabled` (`0x000f302c`), `Engine_SetSubmitEnabled` (`0x000f305c`), and `Engine_SetPrepareEnabled` (`0x000f308c`). They write the 16-bit fields at Engine `+0x20`, `+0x22`, `+0x24`, and `+0x26`.

### History, thresholds, and darkening

The source argument and orientation are unused. A null history pointer is replaced with the current destination. The shader then swaps the stored pointer to the current destination, so alternating back buffers read the preceding one; reusing one buffer intentionally observes earlier writes from the same traversal.

Traversal starts at linear destination index `height + 1`. Outer coordinates run from 1 through `width - 2`, inner coordinates from 1 through `height - 2`, with `height` as the linear row stride. Borders and center pixels are not directly written.

Completed is set at the start of each active render. For each interior previous pixel, **any channel above its threshold clears completed and suppresses that pixel's neighbor writes**. Only pixels whose channels are all at or below their thresholds spread ink. Default thresholds are 32. Modes 1, 2, and 3 set blue, red, and green respectively to 255, effectively ignoring that channel; the other thresholds remain 32.

The ink colors are packed from 8-bit `(r,g,b)` values:

```text
mode 1: (0,16,16)
mode 2: (16,16,0)
mode 3: (16,0,16)
default: (16,16,16)
```

For a qualifying center, the current destination's left, right, `+height`, and `-height` neighbors are computed from the corresponding previous-buffer neighbors. Each output channel is independently clamped at zero:

```text
blue  = previous.blue  - (ink & 0x1f) * 8
green = previous.green - ((ink & 0x07e0) >> 3)
red   = previous.red   - ((ink & 0xf800) >> 8)
```

If completed remains set, the shader reenables pre-update, post-update, submit, and prepare hooks. Active remains set until `StopInk` or `Reset`.

## BlindShader

Blind has no fields beyond `BasicShader`. Its constructor is `0x00030f98`, render is `0x000e3f00`, and destructors are `0x000e4c58`/`0x000e4cb0`.

Unlike every other presenter, Blind treats destination as `uint16_t *` and writes only packed RGB565 halfwords. It never expands to `0x00RRGGBB`; all untouched destination bytes retain their prior values. Source depth is ignored.

The shared fixed-point scale is:

```text
scale = FixedDivide(0x10000, (height - 1) * 0x10000)
```

Orientation one uses outer `y`; every other orientation uses inner `x`. Despite the latter using X, the denominator remains `height - 1`:

```text
attenuation = FixedMultiply(coordinate * 0x10000, scale)
channelOut = FixedMultiply(channelIn * 0x10000, attenuation) >> 16
```

Expanded input channels are red `pixel & 0xf8`, green `((pixel >> 5) & 0x3f) << 2`, and blue `(pixel & 0x1f) << 3`. Each scaled result is truncated to eight bits before RGB565 packing, so values above 255 wrap rather than saturate.

## TwistShader

Twist adds three 32-bit fields to the base: amplitude `+0x14`, offset `+0x18`, and velocity `+0x1c`. `TwistShader_Ctor` (`0x00030f30`) initializes them to 2, 0, and 1. Render is `0x000e43f0`, the RGB565 expansion helper is `0x000e4990`, and destructors are `0x000e4d08`/`0x000e4d60`.

Each render starts a local row offset from the persistent offset and a local row step of +1, regardless of persistent velocity's sign. For each source row:

```text
rowOffset += rowStep
if rowOffset >= amplitude || rowOffset <= -amplitude:
    rowStep = -rowStep
sampleX = clamp(x + rowOffset, 0, width - 1)
output = ExpandRgb565(source[y][sampleX])
```

Depth is ignored. After the frame, persistent offset advances by velocity, and velocity is negated when offset reaches either amplitude boundary. With defaults, the first three frames use row-offset pairs `(1,2)`, `(2,1)`, and `(3,2)`; clamping handles the out-of-range third offset.

## Portable verification

`test_shaders.cpp` checks both orientations, inactive/initial/intermediate/terminal states, exact packed arithmetic, Fade completion and renderer deselection, both Gray keys versus Fade's single key, MotionBlur reset/history reuse/saturation, Ink threshold polarity/modes/history alternation/engine hooks, Blind's 16-bit addressing and channel wrapping, and Twist's multi-frame bounce/clamping.

The normal suite has 22 passing CTest tests, including the SDL application smoke tests. The SDL-independent AddressSanitizer/UndefinedBehaviorSanitizer suite has 16 passing tests. Existing real-resource BSP/SPL/menu/SOJ frames remain unchanged.
