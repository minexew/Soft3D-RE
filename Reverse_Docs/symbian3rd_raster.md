# Symbian 3rd renderer and rasterizer

## Scope

This document records the portable object-rendering path recovered for Stage 7 from `SymbianBin/out.bin` at image base zero. It covers `Renderer`, `Raster`, SOJ material state, projection/clipping, edge walking, and the 100-entry scanline table. BSP lightmapped span rendering is related but uses its own perspective-correct path; see `sbp_format.md`.

The corresponding portable code is `Reversed_Game/src/soft3d/Renderer.*`, `Raster.*`, `Object.*`, and `ObjectRes.*`. Exact directed tests are in `Reversed_Game/tests/test_raster.cpp`.

## Renderer entry points and vertex records

- `Renderer_DrawSequential` (`0x000b106c`) reads consecutive source vertices.
- `Renderer_DrawIndexed` (`0x000b1404`) reads 16-bit indices.
- `Renderer_DrawIndexedTextured` (`0x000b1c38`) combines a primary and optional secondary vertex stream. If color bit `0x08` is present, it clears normal bit `0x02`.
- Primitive types are 4 = list, 5 = strip, and 6 = fan. Strips swap the second and third vertex on odd triangles; fans retain the first vertex.
- `Renderer_PrepareDrawState` (`0x000b5144`) computes model-view state and lighting flags. Lighting requires a normal, no vertex color, and the object lighting flag. Texturing requires UVs and a nonnull texture.
- Matrix-indexed vertices select a 0x40-byte palette matrix. The normal `Renderer_TransformVertex` path uses the raw serialized index directly. Only the indexed-textured stream reader applies `>> 16`; sharing that conversion with the normal path incorrectly collapsed every `oszp_diaodeng_a` lamp vertex onto track zero.

The renderer keeps view at `0x94`, projection at `0xd4`, combined view-projection at `0x114`, and model-view-projection at `0x154`. `Renderer_UpdateViewProjection` (`0x00050e4c`, formerly `FUN_00050e4c`) copies the camera view and recomputes the combined matrix. `Object_UpdateMatrixPalette` and `Renderer_PrepareDrawState` consume the combined matrix; `Object_GetCameraDepth` deliberately uses view only. For palette vertices, X/Y use matrix rows 0/1 and camera Z uses the **entire row 3** `(m3,m7,m11,m15)`, which is homogeneous depth after projection. Non-palette vertices use row 2 `(m2,m6,m10,m14)`. Replacing only the translation or using row 2 for palette geometry is incorrect.

ARM matrix multiplication and vector transformation accumulate each dot product in signed 64-bit form and shift once by 16. The portable `Multiply` and `Transform` now do the same rather than truncating each product separately.

The transformed clip record is 0x34 bytes:

| Offset | Meaning |
|---:|---|
| `+0x00` | Projection/lighting cache flags. |
| `+0x02` | Clip flags. |
| `+0x04`, `+0x08`, `+0x10` | Camera-space X/Y/Z before projection. |
| `+0x0c` | Reciprocal depth written by projection; this is distinct from camera Z at `+0x10`. |
| `+0x14`, `+0x18` | Integer screen X/Y after projection. |
| `+0x1c`, `+0x20`, `+0x24` | Normal XYZ used by lighting. |
| `+0x28`, `+0x2c` | Fixed-point U/V. |
| `+0x30` | Fixed-point palette intensity. |

Ghidra's `RendererClipVertex` structure records this exact 0x34-byte layout. `RendererViewport` is a separate 0x28-byte structure containing integer bounds, integer and fixed half sizes, and adjusted centers.

## Projection and clipping

`Renderer_ClipAndRasterizeTriangle` (`0x000b2d54`) rejects triangles with a shared clip flag and rejects any vertex beyond the far plane. The normal near plane is Z=1.0 and the usual far limit is `0x8c0000` (140.0); the older non-stream transform contains a `0xa00000` comparison, but the stream path uses `0x8c0000`.

Camera-space side tests are `x < -z`, `x > z`, `y < -z`, and `y > z`. Vertices are projected by `Viewport_ProjectVertex` (`0x000d77a8`), called through `Renderer_ProjectVertex` (`0x000b5988`). It uses the global reciprocal table at `0x00160080`:

```text
table[0] = 0x7fffffff
table[n] = floor(0x40000000 / n), n > 0
```

The normalization ladder and truncation are original behavior, including its overflow/wrap cases; it must not be replaced with an ideal floating-point reciprocal. Its final step tests the shifted value with `MOVS value << 1`: when the low 31 bits are zero, the value shifts to zero and selects table entry zero. For example, camera depth 2.0 produces reciprocal `0xfffffc00`; this original overflow is covered indirectly by the rendering fixtures and must not be sanitized away. Normal projection multiplies by integer half-width/half-height at viewport `+0x10/+0x14` and adds the fixed half-size fields at `+0x18/+0x1c` directly. It does **not** subtract `0x8000`. The viewport separately stores adjusted centers `(halfSize << 16) - 0x8000` at `+0x20/+0x24` for other consumers; the near-edge clipper has its own scale/center layout and does use adjusted centers. Applying that adjustment to normal projection was a portable error. Projected X/Y add `0xffff` before an arithmetic `>> 16`.

Near clipping (`Renderer_ClipNearEdge`, `0x000d5f2c`) interpolates camera positions, UVs, and intensity at Z=1.0. Its reciprocal-depth output is the original constant `0x1000000`, not a recomputed ideal value. Screen clipping (`Renderer_ClipPolygonEdge`, `0x000d5504`) runs in left, right, top, bottom order and uses reciprocal-table interpolation. Clip bit 1 is left, bit 2 right, bit 4 top (`screenY < top`), and bit 8 bottom (`screenY > bottom`); the portable implementation previously swapped the two Y planes. Screen X/Y interpolation rounds with `0x8000`; reciprocal depth, UVs, and intensity use fixed-point multiplication without that rounding.

`Renderer_CheckTriangleWinding` (`0x000b4a6c`) computes a signed 32-bit area, rejects zero area, culls negative area in mode 1, and culls positive area in mode 2. Mode 0 renders both windings. Arithmetic wraps as on ARM.

Culling is decided once per source triangle. Without near clipping, ARM tests the original projected triangle before screen clipping. Near clipping defers the test to the first three vertices of the near-clipped polygon. Later screen-clipped fan triangles are not culled independently. The former portable implementation re-culled every generated fan triangle; wrapped areas at large offscreen coordinates could then flip between fan triangles, producing camera-dependent clipped-face artifacts. The lamp's separate animated insect cards are intentional and must not be removed as stray geometry. `Viewport_SetBounds` (`0x000d75e0`, formerly `FUN_000d75e0`) establishes the viewport bounds, half sizes, and adjusted centers.

Stage 13 adds the previously uncalled `Camera_SetViewport` (`0x000fd41c`) and renderer viewport propagation. Right/bottom are `left + width` and `top + height`; integer halves use `>> 1`, fixed halves are those integers shifted left 16, and centers are `origin + half - 0x8000`. Normal vertex projection uses unadjusted halves, while point projection and near intersections use adjusted centers. Screen clipping uses the configured left/right/top/bottom rather than assuming a zero-origin full-screen viewport. Synthetic tests cover a negative origin and propagation from camera to renderer; all original full-screen hashes remain unchanged.

## SOJ material state

The parsed material remains 0x34 bytes. The renderer-facing names are:

| Offset | Recovered meaning |
|---:|---|
| `+0x00` | `directionalScale`: scales positive directional-light contribution. |
| `+0x04` | `ambientIntensity`: base/ambient contribution. |
| `+0x08` | RGB565 flat color. |
| `+0x0c` | `cullMode`, not the RGB blend mode. |
| `+0x10` | `blendMode`: 0 copy, 1 = 75% destination/25% source, 2 = 50/50, 3 = 25% destination/75% source. Selector `0x00ff` requests detail alpha and is converted to zero. |
| `+0x14` | Texture-present flag. |
| `+0x16` | Detail-texture flag. |
| `+0x18` | Four packed texture-name words. |
| `+0x28`, `+0x2c` | Per-render U/V texture-scroll velocities. |
| `+0x30` | Coordinate mode: 3 generates UVs from transformed normals; 0/4 use stored UVs plus offsets. |

Only blend level zero writes depth by default. `ObjectMaterial_Update` (`0x000ed6d4`) advances and bounces the runtime intensity between its limits, adds the U/V speeds once per rendered surface, and wraps at positive/negative texture-size limits with one subtraction/addition. The runtime material starts with intensity and minimum `0x1999`, maximum `0x10000`, and zero speed.

For lighting, `Renderer_ComputeVertexLighting` (`0x000b4be8`) adds only a positive directional term, clamps the base to the renderer minimum, and computes:

```text
if base < 0x10000: intensity = (0x10000 - base) * 0x1f00
else:              intensity = 0
```

This inverted value is interpolated as a palette selector. The raster edge setup converts intensity with `>> 16`, and the textured loop applies another `>> 8`; this double scaling is original behavior. Unshaded textured paths instead use palette level 15.

Mode-3 coordinates use the model-transformed normal:

```text
u = fixedMultiply((normalX >> 1) + 0x8000, textureFixedWidth)
v = fixedMultiply((normalY >> 1) + 0x8000, textureFixedHeight)
```

## Fixed-point triangle and span walking

`Raster_PrepareTriangle` (`0x000ca29c`) adds the depth bias (`input bias >> 8`) to reciprocal depth and clamps each value to at least `0x200`. `Raster_SelectEdgeOrder` (`0x000ca600`) chooses the edge ordering from signed area and Y ordering. The clipped-polygon path uses `Raster_PrepareClippedPolygon` (`0x000cba40`), `Raster_SelectPolygonExtrema` (`0x000cbd38`), and `Raster_RasterizePolygon` (`0x000cc24c`).

`Raster_ComputeAttributeDeltas` (`0x000cdcb8`) computes affine X/Y deltas for reciprocal depth, U, V, and palette intensity. It uses the reciprocal table and fixed shifts/wrapping; there is no perspective divide in the SOJ UV inner loops. BSP lightmapping is the separate perspective-correct implementation.

`Raster_SetupEdge` (`0x000ce170`) and `Raster_ComputeEdgeStep` (`0x000c93ac`) calculate an integer X step and a nonnegative remainder using floor division. Each emitted span starts at the current integer edge positions. `Raster_EmitScanline` (`0x000ce27c`) advances X and all attributes after emitting the row. A nonnegative error uses X step + 1 and subtracts the edge height from the error. The inner-loop pixel count is `rightX - leftX`; zero and negative counts emit nothing, and the right edge is excluded.

ARM selects edge tables using both the original signed area and the Y-order swaps, including dedicated flat-top/flat-bottom cases. The portable sorter must therefore determine left/right span direction from the **post-sort** signed area. Using the pre-sort area reverses spans when sorting swaps an odd number of vertices. The real lamp cable exposes this as a positive-area, flat-bottom triangle whose sorted winding is negative; the old code rejected every cable scanline. Attribute interpolation still uses the original vertex order and original area.

Each scan record is 0x18 bytes:

```text
s32 rightX
u32 *destination
s32 reciprocalDepth
s32 u
s32 v
s32 intensity
```

The destination word contains RGB565 in bits 0–15 and reciprocal-depth information in bits 16–31. The depth value written by the object rasterizer is:

```text
depthWord = ((reciprocalDepth & 0x00ffffff) >> 8) << 16
```

## Scanline selector table

The table at `0x0015b650` has 100 two-word descriptors. `Raster_PrepareTriangle` computes:

```text
selector = 0
if depthWrite: selector += 50
if depthTest:  selector += 25

if textured:
    selector += 5
    if colorKey: selector += 10  // total texture/key group offset 15
    if shading:  selector += 5

if detailTexture: selector += 4
else if blendMode is 1..3: selector += blendMode
```

Thus each five-entry texture group consists of copy, blend 1, blend 2, blend 3, and detail-alpha. Flat groups have no meaningful detail operation and their unused table slots contain aliases. The portable implementation evaluates the same state combinations without reproducing the original function-pointer dispatch overhead.

Depth behavior is independent:

- No test/no write: replace or blend RGB and preserve the old high halfword.
- Test only: compare the entire old 32-bit pixel with the new depth word, then alter only RGB. The old depth is preserved.
- Write only: replace or blend RGB and write the new depth without a comparison.
- Test and write: compare, then replace/blend RGB and write depth.

Most comparisons are unsigned full-word `oldPixel <= depthWord`. Since the low RGB half participates, equal-depth pixels with different colors need not compare equal. The optimized textured-copy selectors 80 (`0x000c3b24`) and 85 (`0x000c4a18`) instead use a signed comparison; this distinction is covered by a directed test.

Color-key paths compare the serialized texture index with STX's transparent index before palette lookup. Detail paths sample the secondary texture at the same computed offset and use its byte as 0–32 alpha. Alpha 0/1 preserves destination RGB, 2–30 interpolate packed RGB565, and 31+ copy source RGB. Detail alpha takes precedence over the ordinary blend field. Depth writes still occur when detail alpha preserves RGB.

## Corrected indexed-texture addressing

The ARM listing at `Raster_InnerLoop05_TextureCopy` (`0x000b60c8`, formerly `Raster_InnerLoopIndexedTexture`) proves that the final right shift applies to the **whole sum**:

```text
maxU = (textureWidth - 1) << 16
maxV = (textureHeight - 1) << 16
sourceOffset = (((v & maxV) << textureWidthShift) + (u & maxU)) >> 16
```

The older notes incorrectly shifted only U before adding the shifted V. That error caused invalid host-side offsets when reconstructed literally. V still selects serialized rows directly; there is no vertical inversion.

`Texture_GetPaletteEntry` (`0x000c8024`) reads the extended palette at:

```text
paletteOffset = ((textureIndex << 5) | intensity) * 2
```

Texture `+0x54` is therefore the palette allocation, not an unidentified secondary allocation. STX `+0x45` enables the color key and `+0x46` stores its index.

## Portable verification

`test_raster.cpp` checks exact span coverage, directed top- and bottom-plane clipping pixels, post-sort winding for a positive-area flat-bottom triangle, original-triangle winding/culling, near/far rejection, sequential/indexed/secondary-stream entry points, list/strip/fan topology, reciprocal-depth packing, independent depth test/write behavior, signed optimized loops, all four ordinary blend levels, texture wrapping and offsets, mode-3 UV generation, color keys, palette lighting, detail alpha, and the selected state matrix. Ordinary synthetic projection fixtures use depth 3.0 so they do not accidentally collapse through the original depth-2.0 reciprocal overflow. `test_effects.cpp` verifies the final direct-axis-column layout, fourth-row palette depth, single-shift matrix/vector accumulation, the real lamp's five surfaces and intentional insect geometry, and portable regression hashes `0x85d115c5` at time 0 and `0xf347549f` at time 1200. `test_spl.cpp` checks both the unprojected view and combined view-projection matrices. `test_bsprender.cpp` checks the exact ARM matrix layout for all five serialized lamp instances. All 25 CTest tests pass in the persistent Debug build. Fixed division uses multiplication by `0x10000` rather than an undefined negative left shift while retaining the same quotient.

The supplied lossy `SymbianMenuFrames` sequence was compared visually without hashing it. The corrected pipeline improves several views, but frame 181 still has a visible lamp-size/height mismatch. Full pixel parity is therefore **not yet established**, despite the ARM-confirmed matrix rules above.
