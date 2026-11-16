# Symbian 3rd renderable effects and fixed-point support

## Scope

This document covers the four engine `Renderable` implementations completed in stage 9: `Lensflare`, `BillboardGroup`, `FakeShadow`, and `GunFire`. These are scene effects, not `BasicShader` presentation effects; the latter are documented in `symbian3rd_shaders.md`. Object/model runtime details are in `soft3d_models_animations.md`.

The portable implementations are `Reversed_Game/src/soft3d/Effects.*`, with support in `Fixed.*`, `Camera.*`, `Renderer.*`, and `Object.*`. Tests are in `Reversed_Game/tests/test_effects.cpp`.

## Shared rendering model

All four classes inherit `Renderable` and use the ordinary renderer/raster path. The original `Renderer_PrepareDrawState` computes:

```text
combined = model * viewProjection
```

Object animation builds matrix palettes with the same view-projection composition. The portable renderer separates view transformation from its existing viewport projection, so its equivalent palette is:

```text
palette = sampledAnimation * model * view
```

The camera roll matrix is not a second view matrix. `Camera_UpdateRoll` (`0x000fcf48`) computes it from target-minus-position X/Z and `Camera_ArcTangent`; BillboardGroup and GunFire use that roll as the model transform before the renderer applies the current view-projection state.

Resource and renderer pointers are non-owning. The original objects retain both `TextureRes` and embedded `Texture` pointers where applicable. Portable setters also accept direct texture pointers for isolated tests and host-managed resources.

## Lensflare

### Layout and API

`Lensflare` is 0x74 bytes. Its source vector is at `+0x18`, followed by five 16-byte elements at `+0x24`. Each element contains enabled, fixed-point offset, `Texture *`, and `TextureRes *` fields.

| Function | Address |
|---|---:|
| `Lensflare_Ctor` / duplicate | `0x000ee634` / `0x000ee580` |
| `Lensflare_LoadElement` | `0x000ee6e8` |
| `Lensflare_ClearElements` | `0x000ee798` |
| `Lensflare_Render` | `0x000ee7f8` |
| `Lensflare_Dtor` / deleting destructor | `0x000eebd8` / `0x000eeb80` |

`LoadElement` accepts indices 0–4, resolves a level-3 texture from the UI directory, and enables the element. Clearing only resets enabled flags; it does not release resources.

### Projection and depth

`Renderer_ProjectPoint` (`0x000b3bb4`) transforms by model-view-projection and rejects Z below 1.0. It computes reciprocal depth with numerator `0x1000000` (256.0), applies viewport X/Y scales, and shifts reciprocal depth right by eight. Viewport centers include the original `-0x8000` half-pixel adjustment.

`Lensflare_Render` requires integer screen X/Y inside the target and:

```c
storedDepth <= projectedDepth + 100;
```

`Raster_GetDepth` (`0x000eeb08`) returns the high 16-bit half of the packed framebuffer word. When visible, every enabled element is drawn with additive blit mode 1:

```text
elementX = projectedX + (centerX - projectedX) * element.offset
elementY = projectedY + (centerY - projectedY) * element.offset
```

The texture is centered by subtracting half its integer width and height. The portable blitter receives mode 1 explicitly instead of mutating and restoring blitter field `+0x1c`; output is identical.

## BillboardGroup

### Layout and lifetime

The constructor at `0x001120d0` allocates a pointer array for the requested capacity and one four-vertex position+UV stream. Records are 20 bytes:

```text
Fixed halfWidth
Fixed halfHeight
Vec3 position
```

`BillboardGroup_Clear` (`0x00112c7c`) deletes each owned record and the pointer array. Portable code stores equivalent records in a vector and enforces the original fixed capacity.

### Visibility and geometry

`BillboardGroup_Render` (`0x001127c8`) computes camera forward as `target - position` and accepts a billboard when:

```text
(billboard.x - camera.x) * forward.x +
(billboard.z - camera.z) * forward.z >= 0
```

For each accepted record, the model matrix is the camera roll matrix with translation set to the billboard position. Four vertices form a strip:

```text
(-halfWidth, -halfHeight, 0)
(-halfWidth, +halfHeight, 0)
(+halfWidth, -halfHeight, 0)
(+halfWidth, +halfHeight, 0)
```

The original UV words remain zero. Rendering uses the selected texture and blend mode, no lighting, cull mode 0, and two triangles. It restores blend 0, cull 2, and lighting 1.

## FakeShadow

### Geometry

`FakeShadow_Ctor` (`0x000f0624`) starts with empty geometry, identity matrices, and black. `FakeShadow_Initialize` (`0x000f06e8`) creates twelve position-only vertices once, in the XZ ground plane. The ring constants, scaled by radius, are:

```text
(-0x14000, 0)       (-0x11999, +0xa666)  (-0xa666, +0x11999)
(0, +0x14000)       (+0xa666, +0x11999)  (+0x11999, +0xa666)
(+0x14000, 0)       (+0x11999, -0xa666)  (+0xa666, -0x11999)
(0, -0x14000)       (-0xa666, -0x11999)  (-0x11999, -0xa666)
```

Each pair is X/Z. `FakeShadow_ScaleXZ` (`0x000f11d4`) multiplies X and Z without changing Y. `SetColor` packs RGB888 into RGB565.

### Rendering

`FakeShadow_Render` (`0x000f1328`) multiplies primary and secondary matrices, then draws a triangle fan with ten triangles using:

- lighting off and flat shadow color;
- no texture and cull mode 0;
- depth writes off;
- depth bias `0x12c0000`.

It restores cull 2, zero depth bias, depth writes on, and lighting on. A test rotates the XZ plane into view; an unrotated ground plane is correctly edge-on to the default camera.

## GunFire

`GunFire` is embedded in the game player in the original, but its renderable behavior is reusable and does not depend on player state. Its four position+UV vertices are:

```text
position (-1,-1,0), UV ( 0,32)
position (-1,+1,0), UV ( 0, 0)
position (+1,-1,0), UV (32,32)
position (+1,+1,0), UV (32, 0)
```

Positions and UVs are signed 16.16 fixed point.

| Function | Address |
|---|---:|
| `GunFire_Ctor` / duplicate | `0x000377a0` / `0x000371c0` |
| `GunFire_LoadTextures` | `0x00037d80` |
| `GunFire_Render` | `0x00037df4` |
| `GunFire_Dtor` / deleting destructor | `0x0003e510` / `0x0003ebd0` |

The loader resolves `zp_huoguang_a` and `zp_huoguang_a_alp` at cache level 2. Rendering occurs only while the frame counter is positive. It uses camera roll plus trigger position, primary/detail textures, no lighting, coordinate mode 0, zero texture offset, blend 0, cull 0, and a two-triangle strip. Cull 2 and lighting 1 are restored, then the counter decrements once per rendered frame. The portable `Trigger` defaults to two frames and direct texture injection is available for hosts that do not use the hardcoded resource names.

## Fixed-point trigonometry

`FixedSin` (`0x00061b38`) uses the 2049-entry quarter-wave table at `0x001a0080`. Angles use 0x2000 units per revolution. The table's peak is 65535, not 65536. Quadrants select direct or mirrored indices and optional negation. `FixedCos` (`0x00061ac4`) is sine at `angle + 0x800`.

`FixedArcTangent` (`0x00125488`) folds values outside [-1,1] through reciprocal and evaluates an odd polynomial through x^21. Coefficients are:

```text
5555, 3333, 2492, 1c71, 1745, 13b1, 1111, 0f0f, 0d79, 0c30
```

The final multiplier is `0x0517cc18`. In particular, `FixedArcTangent(0x10000)` is `0x41d`, not an idealized `0x400`. `Fixed.cpp` and `FixedSinTable.inc` preserve these values directly rather than relying on host floating-point trigonometry.

`FixedVec3_RotateX/Y/Z` (`0x001281b4`, `0x001282cc`, `0x001283e4`) and `FixedVec3_RotateEuler` (`0x001284fc`) preserve original multiplication order and intermediate truncation. `Matrix4_RotateY` (`0x0011e1bc`) constructs camera roll.

## Matrix and raster corrections

The portable `Multiply` previously formed an outer product instead of a matrix dot product. It now computes, for each column and row:

```text
sum(left[index][row] * right[column][index], index = 0..3)
```

This matches `0x0011ac98`. Existing real BSP/SPL/menu hashes remain unchanged.

`Texture_Resize` (`0x000d7ff8`) uses its `newType` argument only to select bytes per pixel; it does not store that argument in texture field `+0x3c`. The existing type byte is preserved while palette/pixel allocations are reset. The palette at `+0x54` is the extended intensity palette, not an unknown secondary allocation. Stage 9 found no additional consumer-backed STX/SOJ/SAI field requiring a format-layout change.

`Raster::DrawSpan` now clips negative and oversized X ranges and rejects out-of-range Y before framebuffer access. Attribute values advance by the number of clipped pixels. This is defensive portable memory safety: visible arithmetic is unchanged, but invalid transformed geometry can no longer read or write past the allocation. An optimized-build crash in the real menu was traced to the former one-past-end access with Valgrind.

## Verification

`test_effects.cpp` covers:

- exact sine/cosine, arctangent, and camera-roll values;
- texture extended-palette cleanup, type preservation, and allocation size;
- Lensflare exact additive pixels, placement, element lifetime, and depth rejection;
- billboard capacity, placement, and back-face rejection;
- shadow vertex layout, orientation, exact flat-color pixels, and scaling;
- GunFire UV layout, exact textured pixels, and two-frame countdown;
- Object animation pause/direction/loop state and exact palette composition;
- real `cg_bully_tuomao_a` SOJ loading/materials/rendering and `cg_bully_cc_aa` SAI playback through `Object`.

`test_raster.cpp` additionally covers an offscreen triangle spanning negative X, oversized X, and out-of-range Y. The complete normal and sanitizer CTest suites and the standalone SOJ converter suite are part of stage-9 validation.
