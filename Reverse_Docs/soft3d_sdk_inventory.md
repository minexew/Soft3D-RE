# Soft3D SDK native-to-portable inventory

## Session 29 status and evidence rules

This is the Phase D inventory and exclusion audit for the Symbian 3rd `out.bin` program (`/7days.bin` in the Ghidra project). Portable source was reviewed at revision `9663d705619bdada2e3f1c49300a67a7c91c9f95`. Addresses below are the corrected native offsets already used by the loaded program; no additional `0x8000` bias is applied.

The audit covered:

- all **153 RTTI classes**, **153 vtables**, and **1,734 vtable slots** catalogued in `symbian3rd_rtti.md`;
- all **462 functions with zero direct references** in the current database;
- called and uncalled scalar/vector/matrix/geometry families, because a zero-caller-only search misses useful APIs such as `FixedArcSine`, fixed-to-integer conversions, determinants, and plane helpers;
- public declarations under `Reversed_Game/src/soft3d`, the Soft2D headers and inline bodies under `Other/S2DSDK`, and the existing subsystem ledgers.

The current database reports 4,829 functions including external/thunk support; the stage-16 local-code count was 3,940. These counts measure different sets and are not evidence of new game code. The high-address data/false-function exclusion from stage 16 remains unchanged.

**Session 29 does not implement the missing APIs.** Their native semantics, ownership, disposition, and test requirements are recorded here for sessions 30–33. “Missing useful API” means there is native evidence for a reusable facility and no equivalent public portable API; it does not mean a stage-16 reachable-behavior test was failing.

Dispositions use the roadmap categories: **implemented**, **missing useful API**, **original empty**, **duplicate compiled body**, **game-specific**, **platform-specific**, **runtime/vendor**, **data/false function**, or **intentionally omitted**. Duplicate bodies are documented but must not become duplicate portable implementations.

### Recovery test codes

Every row names its planned verification. Exclusions require no consumer-style test and use **X**.

| Code | Planned native-derived verification |
|---|---|
| **E** | Existing focused test and real-resource/engine-frame coverage identified in the subsystem document; no new API is needed. |
| **30A** | Session-30 directed scalar tests: exact ARM arithmetic, signed boundaries, overflow/wrapping, zero and negative inputs, reciprocal-table ranges, angle masks, and integer negative exponents. |
| **30B** | Session-30 Color4/Vec2/Vec3/Vec4/quaternion tests: exact components, aliasing, `w` handling, clamp order, short-path interpolation, matrix/axis-angle round trips, zero/degenerate values, and native quirks. |
| **31A** | Session-31 matrix/projection tests: exact 16 entries, composition order, identity, translation/scale/rotation factories, singular determinant behavior, reflection, and project/unproject consistency. |
| **31B** | Session-31 geometry tests: exact plane distances/intersections, five-plane frustum rejection, plane-quad winding/bounds, polygon classify/split/clip, empty/degenerate clouds, and repeated ownership-safe operations. |
| **32A** | Session-32 consumer-style resource/renderer tests through public headers: real STX resources, tiling and teardown, exact sprite pixels/clipping, 16/32-bit fills, and 32-entry dynamic-light ownership/capacity. |
| **33A** | Session-33 utility tests: rectangle edge inclusivity/intersection, input reset-on-change, sound reset/reuse, dialog links/builders/interpolation/current visual, and exact repeated-call behavior. |
| **X** | No new API or test is justified. Do not merge native duplicates or fill empty/runtime/platform routines; retain existing regression coverage. |

## RTTI and portable-owner crosswalk

No unnamed RTTI class remains. Value-type math, Color4, quaternion, Plane, PointCloud, ScreenRect, and AspectRatio have no RTTI records and therefore require body-level review rather than a class-name search.

| Native classes/facilities | Portable owner and disposition | Evidence / test |
|---|---|---|
| `s3dBase`, `s3dIO`, `s3dLog`, `s3dFilePak`, `Resource`, `ResManager`, `AudioData`, `ObjectRes`, `Animation`, `TextureRes`, `Texture`, `s3dImageLoader` | Corresponding `src/soft3d` classes: **implemented**. Useful uncalled additions are listed separately below. | RTTI/vtables plus recovered layouts and parsers in `filepak_api.md`, `resmanager_api.md`, `resource_parsing_api.md`, and `symbian3rd_image.md`; **E**. |
| `Engine`, `GameEngine`, `Input`, `s3dTimer`, `s3dVideo`, `VID_impl`, `s3dAudioDevice`, `s3dMemInfo`, `s3dExit` | `Engine*`, `Input`, `EngineClock`, video/audio backends, and memory shells: **implemented**, except explicitly listed uncalled input APIs. Symbian backend bodies are **platform-specific**. | `gameengine_api.md`, `symbian3rd_input.md`, `symbian3rd_audio.md`, `symbian3rd_video.md`, and `symbian3rd_debug.md`; **E**. |
| `Raster`, `Renderer`, `BasicShader` and six shaders, `Renderable`, `Object`, `Bsp`, `Lensflare`, `BillboardGroup`, `FakeShadow`, `GunFire`, `CameraSpot`, `DummySpot` | `Raster*`, `Renderer*`, `PresentationShaders*`, `Object*`, `Bsp*`, `Effects*`, `CameraSpot*`: **implemented**, with uncalled candidates in sessions 31–32. | RTTI slots and `symbian3rd_raster.md`, `symbian3rd_shaders.md`, `soft3d_models_animations.md`, `sbp_format.md`, and `symbian3rd_effects.md`; **E**. |
| `Stream`, `InputStream`, `InputDataStream`, `InputFileStream`, `OutputStream`, `OutputDataStream`, `OutputFileStream` | `Stream.*`: **implemented**, including uncalled Vec2/Vec4 big-endian helpers and output assignment/append operations. | `symbian3rd_streams.md` and `test_stream.cpp`; **E**. |
| `SimpleScript`, `ScriptInterface` | `SimpleScript.*`: **implemented**. `GameSI` is **game-specific** and lives in `src/game`. | `sst_format.md` and `gamesi_matrix.md`; **E**. |
| `MsgReceiverInterface`, `MsgReceiver`, `GameUnit`, `GameUnitManager`, `VisUnit`, `PhysicUnit`, `Creature`, `Things`, `State<Player>`, `StateSet<Player>`, `StateMachine<Player>` | Generic bases and `StateMachine.hpp`: **implemented**. Concrete player states and descendants are **game-specific**. | `symbian3rd_units.md`, `symbian3rd_mapunits.md`, and `symbian3rd_player.md`; **E**. |
| `Dialog`, `DialogManager`, `Font`, `StringManager`, `GameUIDlg`, menu dialog classes | Generic `Dialog*`, `UiManager*`, `Font*`, `ScreenBlitter*`, `MenuDialogs*`, and generic `GameUIDlg`: **implemented**. Concrete GameUI behavior and menu content are **game-specific**. Uncalled generic dialog/rectangle candidates are in session 33. | `sdt_format.md`, `sbn_format.md`, `symbian3rd_gameui.md`, and `symbian3rd_input.md`; **E**. |
| `Scene`, `GameSI`, `MyGameUnitMgr`, `Player`, `Sword`, all interaction triggers, all map-unit variants, all enemy variants, `Boss`, and concrete menu/GameUI classes | `Reversed_Game/src/game`: **implemented as game code**, not reusable Soft3D API. Duplicate constructors/destructors are **duplicate compiled body**. | Stage 1–16 game ledgers; **X** for SDK inclusion and **E** for behavior. |
| `ClientSession`, `MyClientSession`, `SessionDevice`, `HttpSession`, `TCPSession`, `CIMEI`, `CPhoneObserver`, `CSmsSessionObserver`, socket/phone/mixer active objects | License behavior is **game-specific**; device/network/SMS/thread implementations are **platform-specific**. Portable interfaces and test/Linux/SDL/dummy policies are not new native SDK classes. | `symbian3rd_license.md` and `symbian3rd_audio.md`; **X**. |
| `CSymbian3rdApplication`, `CSymbian3rdDocument`, `CSymbian3rdAppUi`, `CSymbian3rdAppView`, Symbian observer interfaces | **Platform-specific**, intentionally not reproduced as Soft3D. | RTTI and Symbian entry-point evidence; **X**. |
| `XLeaveException`, C++ ABI/external types, imported framework classes | **Runtime/vendor**. | Imports and RTTI records; **X**. |

## Implemented native API crosswalk

These entries prevent already recovered behavior from being mistaken for missing work. Addresses with multiple compiled bodies are represented once in portable source.

| Native address(es) and function family | Portable equivalent | Disposition / test |
|---|---|---|
| `0x00010d60`–`0x0001dfdc`, `0x00061ac4`–`0x00061dcc`, `0x000e7a88`, `0x00124244`–`0x001258e0`: 16.16 copy/set/arithmetic/comparison/shift/sqrt/inverse-sqrt/trigonometry/arc tangent/arc sine/arc cosine | `Fixed.hpp` scalar operations, `FixedSin/Cos/Tan`, `FixedArcTangent`, `FixedArcSine`, and `FixedArcCosine` | **implemented**; **E** plus Session-30 directed fixtures. |
| `0x00010f08`–`0x00013c7c`, `0x00127a04`–`0x0012912c`: Vec3 copy/set/zero/add/subtract/scale/dot/cross/length/normalize/rotation/affine transform | `Add`, `Subtract`, `Scale`, `Dot`, `Cross`, `Length`, `Normalize`, `Rotate*`, `Transform` | **implemented**; **E**. Distinct unused variants are listed below. |
| `0x00061a8c`, `0x00061bf8`, `0x00061ca0`, `0x000d74c0`: Vec2 zero/length/dot/copy | `Vec2` value zero/copy plus public `Length`, `Dot`, `SquaredLength`, and `Normalize` | **implemented**; **30B**, without changing current game calculations. |
| `0x0011ac98`, `0x0011b110`, `0x0011b874`, `0x0011d62c`, `0x0011e1bc`, `0x0011dc28`, `0x0011ff7c`, `0x0012116c`, `0x001214b8` | `Matrix4::Identity`, `Multiply`, `Transpose`, `TranslateMatrix`, `RotateXMatrix`, `RotateYMatrix`, `LookAt` | **implemented**; **E**. Missing factories/inversion are below. |
| `0x00011278`, `0x000139cc`, `0x00013b30`, `0x00013c44`, `0x0002205c`, `0x000223a0`, `0x0006df60`, `0x00126968`–`0x00126b6c`, `0x0012a564` | `Geometry.hpp`: copy/reset/translate/contains/overlaps/expand/center/segment-bounds | **implemented**; **E**. The native segment-bounds function was renamed `Segment_IntersectsBounds`. |
| `0x000838e0`–`0x000883fc` | `BezierCurve3.*`: degree elevation, evaluation/derivatives, collection editing, transforms, timing, SPL load | **implemented**; **E** (`test_spl.cpp`). Ctor/dtor/copy/identity duplicates are not separate APIs. |
| `0x000a0348`, `0x000dc450`, `0x000dc478`, `0x000df2c0`, `0x000eeca0` area, `0x000f1270`, `0x000f5460`, `0x00113780` | FilePak entry count, font counts, file-based SBN load, FakeShadow color, indexed music/sound playback | **implemented**, except `Input_SetAnyKeyDown` at `0x000eeca0`, which is a gap below; **E**. |
| `0x000a550c`, `0x000a55ac`, `0x000a560c`, `0x00113310`, `0x001170e0`, `0x00117768`, `0x0011810c`–`0x001194cc` | AudioDevice voice controls/shutdown, SoundInstance volume, Stream validity and Vec2/Vec4 IO | **implemented**; **E**. |
| `0x000e5a0`/`0x000ea5a0` area, `0x000eaf54`, `0x000ec158`, `0x000ec1dc`, `0x000ec290`, `0x000da050` | Object Euler rotation, bone transform, surface/vertex/triangle/depth-bias accessors | **implemented**; **E**. |
| `0x000efc60`, `0x000efe64`, `0x000f8248`, `0x000f8344`, `0x000f8c70`–`0x000fbdcc`, `0x000fd138`, `0x000fd41c` | Manager queries, Dialog blits/accessors, Camera projection/viewport | **implemented**; **E**. Dialog builders and links are separate gaps below. |

## Session 30: confirmed scalar, Color4, vector, and quaternion gaps

### Scalar and conversion utilities

| Native address / owner | Native evidence | Disposition | Planned test |
|---|---|---|---|
| `0x0009a170` `IntPow` | Integer exponentiation by repeated multiplication; exponent zero returns 1, negative exponent returns integer `1 / pow`, and `0` to a negative power returns 0. No S2D declaration is assumed. | **missing useful API** | **30A**: bases -2/0/1/2, exponents -3/-1/0/1/2, signed wrap. |
| `0x000b5750`, `0x000b58ec`, `0x000cf168`, `0x00112054` | Arithmetic `value >> 16` floor conversion. | **missing useful API**; additional bodies are **duplicate compiled body** | **30A**: positive/negative fractions and extremes. |
| `0x000d7588`, `0x000d75ac` | `(value + 0xffff) >> 16`, i.e. ceil; the wrapper name `ToIntRound` is not mathematical round. | **missing useful API** | **30A**: exact integers, +/-fraction, overflow at the addition. |
| `0x000afcb8` | `(value >> 16) & 0xff`. | **missing useful API** | **30A**: sign and byte masking. |
| `0x000afce8` | S2D `finv` counterpart: `0x40000000 / (value >> 2)`. | **missing useful API** | **30A**: compare S2D inline and native signed boundaries. |
| `0x001069a8`, `0x001069f8`, `0x00106b90`, `0x00106d64` | Signed and unsigned reciprocal-table normalization variants with explicit range shifts; currently duplicated as private BSP helpers, not a public SDK API. | **missing useful API**; preserve consumer-specific duplicate native bodies separately in the ledger | **30A**: every range boundary, zero/sign behavior, exact table index/shift. |
| `0x00125050` | Clamped arc-sine polynomial using the recovered coefficients and signed angle adjustment; arc cosine at `0x00125454` delegates to it. | **missing useful API** | **30A**: clamp outside [-1,1], coefficient fixtures, quadrant signs. |
| `0x001258e0` | Extracts signed high 16 bits as the angle result. | **missing useful API** | **30A**: sign extension and masks. |
| `0x0011b370`, `0x0011b708`, `0x0011b804` | Matrix 3x3/2x2 determinant helpers used by inversion; `FixedDet2` is a small determinant primitive. | **missing useful API** | **31A**: exact fixed-point determinants and singular zero. |

### Soft2D Color4 leftovers

The ARM bodies independently match S2D `Color4` semantics. They are not generic Vec4 arithmetic: contrast and interpolation clamp all four components to `[0,1]`. No native `Color4AdjustSaturation` body was found, so it must not be invented from the S2D header.

| Native address / owner | Native evidence | Disposition | Planned test |
|---|---|---|---|
| `0x00125914` `Color4_Clamp`; duplicate `0x001261f4` | Per-component ordered `>1`, else `<0`, clamp. | **missing useful API**; duplicate body documented | **30B**: below/inside/above range and aliasing. |
| `0x00125bb8` `Color4_Negate`; duplicate `0x001263f4` | Returns `(1-r,1-g,1-b,1-a)`, matching S2D unary color negation. | **missing useful API**; duplicate body documented | **30B**: all components and out-of-range input. |
| `0x00125cc4`, `0x00125e70`, `0x001264c4`, `0x00126658` `Color4_AdjustContrast` | `0.5 + (input-0.5)*contrast` for RGB, alpha copied, then clamped. | **missing useful API**; three **duplicate compiled body** instances | **30B**: exact constants, alpha preservation, clamp order. |
| `0x00126060` `Color4_Lerp`; duplicate `0x00126830` | `a + (b-a)*fraction` on four components followed by clamp. | **missing useful API**; duplicate body documented | **30B**: endpoints, extrapolation, aliasing, clamp order. |

### Vec2, Vec3, and generic Vec4

| Native address / owner | Native evidence | Disposition | Planned test |
|---|---|---|---|
| `0x00129200`, `0x0012968c` | Vec2 inverse-square-root normalization and squared length. | **missing useful API** | **30B**: zero, ordinary, overflow-prone values. |
| `0x001292a0` | Vec2 affine matrix transform using matrix rows 0/1 and translation; formerly misnamed dot. | **missing useful API** | **30B**: identity, translation, exact truncation, aliasing. |
| `0x001293c0` | Vec2 projective transform; divides by `m[3]*x + m[7]*y + m[15]`. | **missing useful API** | **30B**: `w` 0/1/other and exact division. |
| `0x0012959c` | Vec2 direction transform without translation; formerly misnamed cross. | **missing useful API** | **30B**: translation must have no effect. |
| `0x00127ffc` | Vec3 linear interpolation with 64-bit product sums shifted once. | **missing useful API** | **30B**: endpoints, negative fraction, truncation. |
| `0x00128074`, `0x00128114` | Component scale/add helpers. ARM uses **input.x for all three outputs**, not per-component y/z; disassembly confirms this original bug. | **missing useful API** | **30B**: explicitly expect repeated-x results; do not “fix” them. |
| `0x00128d5c` | Vec3 direction transform without translation. | **missing useful API** | **30B**: translation independence and aliasing. |
| `0x00128a1c` | Vec3 full 4x4 transform followed by division by computed `w`. | **missing useful API** | **30B**: `w` 0/1/other and exact arithmetic. |
| `0x00128f50` `FixedVec3_Project` | Converts Vec3 to homogeneous Vec4, applies world/view/projection, divides by `w`, then applies viewport. | **missing useful API** | **31A**: identity and real camera matrices, zero `w`, round trip with unproject. |
| `0x0012907c` `FixedVec3_Unproject` | Applies inverses in viewport/projection/view/world order. | **missing useful API** | **31A**: identity and real camera round trip; singular native behavior. |
| `0x001296f4` | Squared Vec3 distance. | **missing useful API** | **30B**: signed deltas and overflow. |
| `0x00129a0c`, `0x0012acb4` | Point-to-ray distance with optional closest point; line wrapper applies an additional square root to the returned length, an observable native quirk. | **missing useful API** | **31B**: parallel/perpendicular/closest-point cases and nested-sqrt fixture. |
| `0x0012a1f8` | Quadrant-aware arc tangent of Vec3 X/Z using arc sine/cosine selection. | **missing useful API** | **30B**: all quadrants, axes, and dominant-component ties. |
| `0x0012ab14` | Slide projection `value - normal * dot(value,normal) * 0x1028f`, used by physics. | **missing useful API** | **31B**: parallel/perpendicular vectors and exact `0x1028f` scale. |
| `0x00126c40`, `0x00126e54`, `0x00126f30`, `0x00127418` | Four-component normalize/length/squared-length/dot compiled duplicates. | **missing useful API**; also **duplicate compiled body** of later Vec4 routines | **30B**: all four components, zero, aliasing. |
| `0x00126d38` | Homogeneous divide only when `w != 1`; otherwise leaves output untouched; writes `w=1` on the taken path. | **missing useful API** | **30B**: taken/untaken paths and preinitialized output. |
| `0x00127000` | Four-dimensional cross product of three Vec4 inputs. | **missing useful API** | **30B**: basis vectors, orthogonality, aliasing. |
| `0x001274ec` | Generic four-component lerp without quaternion sign correction. | **missing useful API** | **30B**: negative dot must not flip the second vector. |
| `0x00127678` | Full Vec4 × Matrix4 transform including `w` and all four output rows. | **missing useful API** | **31A**: identity, translation, nonunit `w`, aliasing. |
| `0x0012afa8`, `0x0012b0a0`, `0x0012b17c`, `0x0012b24c`, `0x001300cc` | Main Vec4 normalize/length/squared-length/dot and range-reduced robust normalization used for clip planes. | **missing useful API** | **30B**: ordinary, zero, huge/small values, exact range reduction. |
| `0x00130414` | Vec4 transform by a matrix inverse; not an inversion routine itself. | **missing useful API** | **31A**: identity and exact homogeneous output. |

### Quaternion family

The common four-component storage is `Vec4`; the quaternion-specific ownership is established by axis-angle, Euler, and matrix conversion arithmetic, not by RTTI.

| Native address / owner | Native evidence | Disposition | Planned test |
|---|---|---|---|
| `0x0012ad38` | Identity `(0,0,0,1)`. | **missing useful API** | **30B**. |
| `0x0012ade8` `Quaternion_Conjugate` | Negates xyz and preserves `w`. | **missing useful API** | **30B**: verify it is not full negation. |
| `0x0012ae8c` `Quaternion_Inverse` | Normalized conjugate: xyz negated, `w` positive, all scaled by inverse length. | **missing useful API** | **30B**: unit/nonunit/zero inputs. |
| `0x0012b320` `Quaternion_FromAxisAngle` | xyz = axis * sin(angle/2), w = cos(angle/2). | **missing useful API** | **30B**: zero/half/full native angle and unnormalized axis. |
| `0x0012b40c` `Quaternion_FromMatrix` | Trace/diagonal branch matrix-to-quaternion conversion; formerly misnamed Slerp. Includes threshold `0x28f` on reciprocal branches. | **missing useful API** | **30B**: each branch, threshold boundaries, exact matrices. |
| `0x0012bbe0` `Quaternion_FromEuler` | Half-angle Euler products with native xyz assignment/sign order. | **missing useful API** | **30B**: zero, individual axes, combined angles. |
| `0x0012bf1c` `Quaternion_Lerp` | Linear interpolation that negates the second quaternion when dot < 0; no spherical interpolation. | **missing useful API** | **30B**: positive/negative dot and no post-normalization assumption. |
| `0x0012c2a4` `Quaternion_ToAxisAngle` | Axis from xyz / sqrt(1-w²), fallback `(0,1,0)` below `0x28f`, angle = `2*acos(w)`. | **missing useful API** | **30B**: fallback threshold, clamping, exact angle. |
| `0x0012c47c` `Quaternion_ToMatrix` | Quaternion products to a 4x4 rotation matrix with homogeneous final row/column. | **missing useful API** | **31A**: identity, basis rotations, comparison with `Matrix4_SetReflection`. |

## Session 30 completion ledger

All **30A/30B** candidates above are now public portable APIs. The tables above retain the Session-29 candidate dispositions that directed recovery; this ledger is the current implementation status. Implementations are in `Reversed_Game/src/soft3d/Fixed.*` and `Color.hpp`, with directed public-consumer tests in `Reversed_Game/tests/test_math.cpp`. Duplicate native bodies map to one portable implementation and are not duplicated in source.

| Native address(es) | Portable API | Disposition and native-derived verification |
|---|---|---|
| `0x0009a170` | `IntPow` | **matches**. Repeated 32-bit multiplication, exponent zero, negative integer exponents, explicit zero base, and wrapped products. If a wrapped nonzero base produces a zero denominator for a negative exponent, the portable guard returns zero instead of entering the external runtime's division-by-zero path; this is an **intentional portable safety difference**. |
| `0x000b5750`, `0x000b58ec`, `0x000cf168`, `0x00112054` | `FixedToInt` | **matches**. Arithmetic high-16 floor conversion, including negative fractions and both signed extremes. |
| `0x000d7588`, `0x000d75ac` | `FixedToIntCeil` | **matches**. Wrapped `value + 0xffff` before the arithmetic shift, including `INT32_MAX -> -32768`. |
| `0x000afcb8` | `FixedToUnsignedByte` | **matches**. High-word byte mask and sign cases. |
| `0x000afce8` | `FixedReciprocalQuarter` | **matches** for nonzero `(value >> 2)`, including signed extremes. A zero shifted denominator returns zero as an **intentional portable safety difference** from calling the imported `__aeabi_idivmod` division-by-zero path. |
| `0x001069f8`, `0x001069a8` | `FixedReciprocalUnsigned16`, `FixedReciprocalSigned16` | **matches**. Every range boundary, table-index truncation, zero sentinel, sign wrapping, and `INT32_MIN`; unsigned zero produces `-4`, signed zero produces `4`. |
| `0x00106b90`, `0x00106d64` | `FixedReciprocalUnsigned17`, `FixedReciprocalSigned17` | **matches**. The distinct 17-bit range ladder and signed wrapper; unsigned zero produces `-2`, signed zero produces `2`. |
| `0x00125050`, `0x001258e0`, `0x00125454` | `FixedArcSine`, `FixedToAngle`, `FixedArcCosine` | **matches**. Clamped polynomial, coefficient sequence, signed high-word extraction, negative-angle adjustment, and arc-cosine delegation. Existing arc-cosine results are unchanged. |
| `0x00125914`, `0x001261f4` | `Color4Clamp` | **matches**. Ordered per-component clamp of all four components and in-place return. |
| `0x00125bb8`, `0x001263f4` | `Color4Negate` | **matches**. `1-component` for RGBA without clamping. |
| `0x00125cc4`, `0x00125e70`, `0x001264c4`, `0x00126658` | `Color4AdjustContrast` | **matches**. RGB contrast about `0x8000`, alpha copy, then all-component clamp. |
| `0x00126060`, `0x00126830` | `Color4Lerp` | **matches**. Four-component interpolation followed by clamp, including extrapolation and value-assignment aliasing. |
| `0x00061a8c`, `0x000d74c0` | `Vec2` default zero/copy | **implemented** by the value type; no unnecessary duplicate algorithm. |
| `0x00061bf8`, `0x00061ca0`, `0x0012968c`, `0x00129200` | `Length(Vec2)`, `Dot(Vec2)`, `SquaredLength(Vec2)`, `Normalize(Vec2)` | **matches**. Separate fixed multiplies/adds, zero normalization, ordinary and overflow-prone values. |
| `0x001292a0`, `0x001293c0`, `0x0012959c` | `Transform(Matrix4,Vec2)`, `TransformPerspective(Matrix4,Vec2)`, `TransformDirection(Matrix4,Vec2)` | **matches**. Native row order, translation behavior, reciprocal-before-output order, and zero/unit/nonunit `w`. |
| `0x00127ffc` | `LerpVec3` | **matches**. Wrapped deltas, one 64-bit product shift per component, negative fractions, and truncation. |
| `0x00128074`, `0x00128114` | `MultiplyComponents`, `AddComponents` | **matches**, including the original repeated-`input.x` behavior for all three outputs. |
| `0x00128d5c`, `0x00128a1c` | `TransformDirection(Matrix4,Vec3)`, `TransformPerspective(Matrix4,Vec3)` | **matches**. Translation independence and exact projective arithmetic for zero/unit/nonunit `w`. |
| `0x001296f4` | `SquaredDistance` | **matches**. Wrapped signed deltas and component squares. |
| `0x0012a1f8` | `Vec3AngleXZ` | **matches**. All four quadrants, axes, strict dominant-component comparisons, and tie handling through arc sine. |
| `0x00126c40`/`0x0012afa8`, `0x00126e54`/`0x0012b0a0`, `0x00126f30`/`0x0012b17c`, `0x00127418`/`0x0012b24c` | `NormalizeVec4`, `Length(Vec4)`, `SquaredLength(Vec4)`, `Dot(Vec4)` | **matches**; duplicate bodies documented. Exact RGBA components, zero, and the native square-root approximation. |
| `0x00126d38` | `DivideVec4ByW` | **matches**. Leaves a preinitialized output completely untouched for `w == 1`; otherwise divides xyz and writes `w=1`, including aliasing and zero `w`. |
| `0x00127000` | `Cross3` | **matches**. Native minor order and output signs, basis result `(0,0,0,-1)`, and orthogonality. |
| `0x001274ec` | `LerpVec4` | **matches**. No quaternion negative-dot sign correction. |
| `0x001300cc` | `NormalizeRobust` | **matches**. Range reduction uses only xyz magnitude but scales all four outputs. Directed fixtures preserve small-input `0x1fffc` inverse-square-root behavior, large shifts, zero, and `w` handling. |
| `0x0012ad38`, `0x0012ade8`, `0x0012ae8c` | `QuaternionIdentity`, `QuaternionConjugate`, `QuaternionInverse` | **matches**. Identity, xyz-only conjugation, normalized-conjugate inverse for unit/nonunit/zero values. |
| `0x0012b320` | `QuaternionFromAxisAngle` | **matches**. Native half-angle units and unnormalized axes. |
| `0x0012b40c` | `QuaternionFromMatrix` | **matches**. Trace and all three diagonal branches, exact off-diagonal signs, and the `0x28f` guard on both sides using wrapped diagonal fixtures. |
| `0x0012bbe0` | `QuaternionFromEuler` | **matches**. Native xyz assignment/sign order and exact half-angle products. Host-math-only expectation differences are separately conditional in the test. |
| `0x0012bf1c` | `QuaternionLerp` | **matches**. Negative-dot second-quaternion negation with no post-normalization. |
| `0x0012c2a4` | `QuaternionToAxisAngle` | **matches**. `0x28f` scale threshold, `(0,1,0)` fallback, clamped arc cosine, and doubled native angle. |

Session-30 handoff: no unresolved scalar/vector candidate remains. The new APIs have no existing callers, so reachable gameplay behavior was not rerouted through them. `FixedArcCosine` now delegates to the recovered arc sine while preserving its exact outputs. One file-local BSP direction helper was renamed to avoid a public-overload collision, and the recovered Vec3/Vec4 lerp and Vec4 normalize APIs use type-specific names; arithmetic and call order are unchanged. The next scoped work is Session 31's matrix/projection/geometry surface, not additional scalar invention.

Session-30 validation passed the complete serial matrix: **40/40 default**, **33/33 SDL-independent core**, **40/40 STDIO logging**, **40/40 host trigonometry**, **40/40 WAR**, **33/33 ASan/UBSan**, and **19/19 applicable English-layout** tests, plus **6/6 SOJ** and **8/8 S3DTEX** Python tests. Host-math expectations distinguish the explicit nonfaithful sine/cosine variant from the native lookup-table values.

## Session 31: matrix, plane, frustum, and point-cloud gaps

| Native address / owner | Native evidence | Disposition | Planned test |
|---|---|---|---|
| `0x0011be64`, `0x0011cc90`, `0x001291c8` | Full cofactor matrix inversion with `1/determinant`; wrapper zero-initializes output. | **missing useful API**; second body is **duplicate compiled body** | **31A**: identity, affine, general, singular determinant (native `FixedDivide` result). |
| `0x0011bbac` | Matrix assignment copy of 16 entries. | **missing useful API** | **31A**. |
| `0x0011d2d4`, `0x0011d3f0` | In-place scale and scale-matrix factory. | **missing useful API** | **31A**: all columns including translation behavior. |
| `0x0011d698` | Identity with explicit translation; uncalled variant of the identity family. | **missing useful API** | **31A**. |
| `0x0011d8d4`, `0x0011de74`, `0x0011e408`, `0x0011e750`, `0x0011e99c`, `0x0011f2a8` | Uncalled rotate-X/Y/Z factory/setter variants and duplicates. | **missing useful API**; duplicates documented | **31A**: exact entries against existing reachable rotation routines. |
| `0x0011f6e0`, `0x0011ff7c` | Euler rotation factories/setters; the setter has callers, the uncalled body is still a useful public operation. | **implemented arithmetic / missing useful API variant** | **31A**: composition order and all 16 entries. |
| `0x00120344`, `0x00120c50` | Uncalled multiply variants using the same storage order as the reachable multiplier. | **duplicate compiled body / intentionally omitted as separate portable APIs** | **X**; retain `Multiply`, **E**. |
| `0x001214b8` | Uncalled LookAt duplicate with the same basis rules as `FixedMat4_LookAt`. | **implemented** | **E**. |
| `0x00121738`, `0x00121a08` | Orthographic matrix with `2/width`, `2/height`, `1/(far-near)`, and native translation. | **missing useful API**; duplicate documented | **31A**: exact matrix and zero-span behavior. |
| `0x00121cc8`, `0x00122040` | Frustum factory with signed `-2` terms and native near/far placement. | **missing useful API**; duplicate documented | **31A**: asymmetric bounds and exact entries. |
| `0x00122674` | Perspective from tangent half-angle, with `m[11]=-1` and positive far*scale term. | **missing useful API** | **31A**: distinguish sign/layout from active Camera projection. |
| `0x00122930`, `0x00122bf0` | Perspective from width/height/near/far with `m[11]=+1` and negative near*scale term. | **missing useful API**; duplicate documented | **31A**: compare active Camera defaults. |
| `0x00122ea4`, `0x00123208` | Viewport matrix combining NDC scale/offset and depth mapping. | **missing useful API**; duplicate documented | **31A**: exact entries and reversed bounds. |
| `0x00123564` | Shear/offset matrix using half-width/height shifts and near/far subtraction. | **missing useful API** | **31A**: odd integer truncation and all entries. |
| `0x00123bf4` `Matrix4_SetReflection` | Reflection from plane `(a,b,c,d)`: `I-2*n*n^T`, translation `-2*d*n`. | **missing useful API** | **31A**: axis and oblique planes. |
| `0x000ec4a4`, `0x00123820`, `0x00123f80` | Matrix construction from axes, rotation axes, and raw 16-value assignment. | **missing useful API** | **31A**: storage order, normalization assumptions, exact values. |
| `0x0008132c`, `0x000cfee8`, `0x00081298` | Plane zero/set constructors and `dot(normal,point)+distance`. | **missing useful API as public Plane type**; arithmetic already exists privately | **31B**. |
| `0x00124118`, `0x00124164`, `0x001241bc` | Plane normal normalization with `d` copied unchanged, point+normal transform using the same matrix, and normal dot. | **missing useful API** | **31B**: verify native non-inverse-transpose behavior. |
| `0x0012ac38` `Plane_FromPointNormal` | Builds `(normal, -dot(point,normal))`; not Vec3 unprojection. | **missing useful API** | **31B**. |
| `0x00129ba4`, `0x00129f3c` | Plane signed distance with optional projected point, and ray intersection with `0x28f` parallelism threshold. | **missing useful API** | **31B**: parallel, behind, threshold, optional output. |
| `0x000cf8ec` | Tests five clip planes and rejects when distance < `-point.w` (the input is treated as Vec3 plus following `w` storage). | **missing useful API** | **31B**: each plane, exact boundary, input layout. |
| `0x0012c9d8`, `0x0012cdd0` `PointCloud_FromPlane` | Allocates four points on a plane from `(normal,d)`, choosing a basis from the dominant normal axis. Not bounds construction. | **missing useful API**; duplicate documented | **31B**: each dominant axis, sign ties, exact winding/center. |
| `0x0012d1c8`, `0x0012d304`, `0x0012fefc`, `0x0012ffc8` | PointCloud deep assignment, reversed copy, allocation, and deep copy. | **missing useful API** | **31B**: independent ownership, empty, repeated assignment. |
| `0x0012d3bc`, `0x0012d470` | First-three-point normalized plane and fan triangulation area. | **missing useful API** | **31B**: winding, area, degenerates. |
| `0x0012d5cc`, `0x0012d648` | Bounds via native invalid sentinel and average center. | **missing useful API** | **31B**: empty count behavior and exact division. |
| `0x0012d75c` | Removes vertices when adjacent normalized edge dot >= `0xffbe`; includes native cleanup quirks. | **missing useful API** | **31B**: collinear/corner cases, count zero, repeated optimize. |
| `0x0012da7c` | Plane classification with epsilon; returns 1 positive, 2 negative, 3 spanning, 4 on-plane. | **missing useful API** | **31B**: all four results and epsilon boundaries. |
| `0x0012dbd4` | In-place positive/on-plane split with edge intersections and axis-plane special cases. | **missing useful API** | **31B**: mixed winding, axis planes, no-positive case, allocation safety. |
| `0x0012e56c`, `0x0012ef20` | Allocating positive/on-plane clip, returning a copy when wholly inside; duplicate compiled body. | **missing useful API**; duplicate documented | **31B**: inside/outside/spanning, ownership, empty result. |
| `0x0012fb18` | Polygon validation/build-plane result codes 0..6 for valid, too few points, small area, large coordinate, short edge, nonplanar, concave. | **missing useful API** | **31B**: one directed fixture per result and thresholds `1.0`, `0x1999`, `0x28f`, `0x20000000`. |

### Native geometry names corrected in this pass

`FixedVec3_Unproject` at `0x0012ac38` was actually `Plane_FromPointNormal`; real unprojection is `0x0012907c`. `FixedVec3_Perpendicular` at `0x0012a564` is the already portable segment/AABB test and is now `Segment_IntersectsBounds`. `Matrix4_SetTransform` at `0x00123bf4` is `Matrix4_SetReflection`. `PointCloud_FromBounds` at `0x0012c9d8` is `PointCloud_FromPlane`. These are evidence corrections, not portable behavior changes.

## Session 32: resource, texture, renderer, and BSP gaps

| Native address / owner | Native evidence | Disposition | Planned test |
|---|---|---|---|
| `0x000e1a00`, `0x000e1aa0`, `0x000e1b40` `TextureTarget` | Engine-neutral texture-target constructor owns screen dimensions/stride/tiling buffer; uncalled STX file loader tiles into a different target width and resets the source texture. | **missing useful API**; constructor duplicate documented | **32A**: real STX, matching/different width, repeated load, failure and teardown. |
| `0x000b446c` `Renderer_RenderTriangle` | Uncalled sprite-style submission: projects one Vec3, configures an optional source rectangle and color, then prepares a scaled quad. It is not the ordinary indexed-triangle entrypoint. | **missing useful API** | **32A**: default/custom source rect, near/far rejection, exact pixels and color. |
| `0x000ce6b8`, `0x000ce804` | Packed 16-bit and full 32-bit horizontal color-span writers. Only the 32-bit body has a caller. | **missing useful API** for the 16-bit variant; 32-bit arithmetic **implemented** | **32A**: exact pixel values, clipping, zero count. |
| `0x000fe03c` `Bsp_AddDynamicLight` | Allocates a 0x18-byte plane/link record, links it at the list head, assigns the previous count as ID, and returns null above 32 entries. | **missing useful API** | **32A**: 0/1/32/33 allocations, IDs, linked traversal, repeated teardown. |
| `0x000ce8a4` | Uncalled internal clipping/pointer setup for texture-target scaled drawing. Portable `ScreenBlitter` and renderer setup already own the corresponding behavior. | **intentionally omitted as public API** | **X**; retain raster/blitter **E**. |
| `0x00102e50`, `0x00102fdc` | Uncalled BSP span fill/texture routines tied to native scanline layout. | **intentionally omitted as separate SDK entrypoints** | **X**; rendering behavior remains covered by `test_bsprender.cpp`, **E**. |

## Session 33: input, sound, dialog, and screen geometry gaps

| Native address / owner | Native evidence | Disposition | Planned test |
|---|---|---|---|
| `0x000eeca0` `Input_SetAnyKeyDown` | Clears key state when the incoming any-key byte differs, then stores it. | **missing useful API** | **33A**: same/different values and clearing semantics. |
| `0x000ef0f0`, `0x000ef1c8`, `0x000ef234`, `0x000ef30c` | Generic Input any-key FIRST_DOWN/PRESS/UP/DOWN_OR_CLICK scans keys 1–23. DOWN and CLICK scans at `0x000ef15c`/`0x000ef2a0` are already implemented. | **missing useful API** | **33A**: each key slot, no keys, mixed states, and DOWN_OR_CLICK short-circuiting. |
| `0x00117108` `SoundInstance_Reset` | Resets volume, voice=-1, position/distance, data/owned flags, and list index=-1 without owner/listen-vector writes. | **missing useful API** | **33A**: exact preserved/cleared fields and reuse after stop. |
| `0x000f8430` | Links two dialogs bidirectionally; null substitutes self. | **missing useful API** | **33A**: null, other, repeated links. |
| `0x000f8508` | Adds a six-field element template to the first negative-ID slot among 20; full table is a silent no-op. | **missing useful API** | **33A**: 0/19/20 entries and field order. |
| `0x000f8690` | Stores the dialog blend mode at native offset `+0x10`. | **missing useful API** | **33A**: all supported modes and render propagation. |
| `0x000f8834` | Reads current element X/Y and emits a key/render operation. | **missing useful API** | **33A**: argument mapping and generated key. |
| `0x000f88b4` | Steps an interpolated key index, renders it, wraps to zero at the end, and returns whether it completed. | **missing useful API** | **33A**: intermediate/end/repeated call and 16-bit key index. |
| `0x000fbd7c` | Lazily allocates and deep-copies the current `DialogVisual`. | **missing useful API** | **33A**: first/repeated assignment and source independence. |
| `0x000fb554` | Empty dialog hook. | **original empty** | **X**: leave empty. |
| `0x000da7e0`–`0x000da89c` `S3dPoint` | Two-int zero constructor and x/y setter with duplicate bodies. | **missing useful API**; duplicates documented | **33A**: trivial layout and set. |
| `0x000da8e0`–`0x000dabdc` | ScreenRect constructors: zero, explicit edges, point+size, copy. | **missing useful API**; duplicates documented | **33A**: field order and point+size arithmetic. |
| `0x000dac38`, `0x000dac74` | Point/SDT-position assignment helpers; they write the first two fields, not a complete rectangle. | **missing useful API** | **33A**: preserve trailing fields. |
| `0x000dacb0`–`0x000daec4` | Offset, mirrored, reverse, and independent-axis offset variants with distinct signs. | **missing useful API** | **33A**: exact per-edge sign tables. |
| `0x000daf4c`–`0x000daffc` | Set left/top/position while preserving width/height. | **missing useful API** | **33A**: negative moves and unchanged extents. |
| `0x000db090`–`0x000db584` | Equality/inequality, empty, zero, inclusive point containment, set, width/height, include (union), clamp (intersection), assignment. | **missing useful API**; wrappers/duplicates documented | **33A**: inclusive edges, inverted/empty rectangles, union/intersection. |

`VisString` constructors/assignment/clear at `0x000ddde0`–`0x000de340` are covered by the C++ value type and are **implemented**, not additional missing algorithms. `Font_GetBaseCharCnt`/`GetLocalCharCnt`, `StringManager_LoadSbnFontFromFile`, Dialog blits/accessors, and Stream Vec2/Vec4 helpers were checked and are **implemented** (**E**).

## Duplicate, empty, runtime, game, and platform exclusions

This section closes the remaining zero-direct-reference candidates. Each row's addresses identify the exact native bodies reviewed; none should be converted into a new reusable API solely because it is uncalled.

| Native bodies | Disposition and evidence | Test |
|---|---|---|
| Game constructor/destructor duplicates at `0x0000d5f8`, `0x00013e10`, `0x00014770`, `0x000151a8`, `0x0001e1e0`, `0x0001ee30`, `0x0002259c`, `0x00022800`, `0x00022d34`, `0x000269c0`, `0x00027228`, `0x00029abc`, `0x00032298`, `0x000371c0`, `0x00038234`, `0x0003f824`, `0x00049bdc`, `0x0004acac`, `0x0004b514`, `0x0004d3a8`, `0x0004e7e4`, `0x0005116c`, `0x00052b40`, `0x00053294`, `0x000536bc`, `0x000540ec`, `0x000541ac`, `0x000546ec`, `0x0005488c`, `0x00055004`, `0x00061df0`, `0x0006e148`, `0x00071cd0`, `0x00075c58`, `0x00075df8`, `0x00077da8`, `0x000780a4`, `0x00079384`, `0x00081884`, `0x00082a40`, `0x0008aa5c`, `0x0008ac24`, `0x0008b53c`, `0x0008ee58`, `0x0008eff4`, `0x0008f4dc`, `0x000934e4`, `0x00093848`, `0x00095730`–`0x000959c0` | **duplicate compiled body** and **game-specific**. Native bodies were already covered by stage 1–16 game reconstruction. | **X / E**. |
| Other uncalled game methods at `0x0000ea8c`, `0x0001edd4`, `0x00027138`, `0x0003b920`, `0x0003b97c`, `0x0003cecc`, `0x0003f7fc`, `0x0003fc04`, `0x0003fce4`, `0x00049e5c`, `0x0004ac80`, `0x0004b4a4`, `0x0004d2a8`, `0x0004dfa4`, `0x0004e6b8`–`0x0004e7c8`, `0x00050700`, `0x000530d4`–`0x00053ed4`, `0x0006505c`, `0x0006e120`, `0x00075d88`, `0x00077ab8`, `0x00078064`, `0x00078c60`–`0x00080f68`, `0x000829a0`, `0x00082ae0`, `0x0008ab8c`, `0x00093758`, `0x00095a50`, `0x00096030` | **game-specific** (license, player, camera, ControlMap, menu, enemies, triggers, debug console), or **original empty** where the body is empty. They are excluded from reusable Soft3D, then separately crosswalked as implemented, missing game API, duplicate, or reserved in `game_object_inventory.md` for Session 34A. | **X / E** for SDK; game recovery tests are assigned there. |
| `0x0009773c`–`0x00099d6c`: narrow/wide `str*`, `mem*`, integer/float conversion routines | **runtime/vendor** C/C++ library support. Their presence is not evidence of a public Soft3D text API. | **X**. |
| `0x0009b44c`–`0x0009eb64`: complete `Fixed64_*` family | **runtime/vendor** eight-byte integer/64-bit arithmetic support, not 16.16 Soft3D fixed point. Multiplication, division, bit operations, float/double conversion, and compiler-generated identities stay out of `Fixed.hpp`. | **X**. |
| `0x00099f1c`–`0x0009a170` `AspectRatio_Ctor`, `SetDimensions`, `Assign` | **platform-specific** GameView screen-descriptor helper owned by the Symbian application view, not a separate engine SDK class. Camera aspect behavior is already implemented. `IntPow` is separately recovered as useful. | **X**. |
| `0x000a03bc`–`0x000a521c`, `0x000a6378`, `0x000a75ac`–`0x000ad50c`, `0x000adb00`–`0x000ae7a8` | **platform-specific/runtime** socket, IMEI, mixer, Symbian descriptor/session, aligned allocation, memory, file, date, phone/SMS functions. `MemTotal`/`MemRemain` already have portable diagnostics; this does not make the Symbian calls reusable SDK. | **X / E**. |
| `0x000a70d4` `S3dVideo_SetTextColor`, `0x000ed160`, `0x000ed440`, `0x00116da0`, `0x00131328`, `0x001325e8` | **original empty** or identity/destructor shells. | **X**. |
| `0x000838e0`–`0x00087ed0`, `0x000aeee4`, `0x000b3560`, `0x000c9558`, `0x000d41d8`, `0x000d7aa8`, `0x000da7e0`–`0x000db550`, `0x000dc744`, `0x000ddde0`–`0x000de340`, `0x000e1a00`, `0x000e2680`, `0x000e3270`, `0x000e3830`, `0x000e8d0c`, `0x000ed090`–`0x000edd18`, `0x000ee580`, `0x000ef6d0`, `0x000fbdcc`, `0x000fc238`, `0x0010a108`, `0x0011490c`, `0x001183ac`, `0x00119654`, `0x00133810` | **duplicate compiled body** or ordinary C++ value/constructor/destructor lifecycle already represented by portable classes. Useful nonduplicate methods from these families are separately listed above. | **X / E**. |
| `0x001120d0`, `0x00113188`, `0x00116c68`, `0x001311d8`, `0x00132480`, `0x001361b0`, `0x00136258` | Constructor/destructor and RTTI-less C++ lifecycle bodies: **implemented** through normal portable object ownership, not additional SDK functions. `SoundInstance_Reset` is a separate gap. | **E**. |
| `0x001378d4`–`0x00138b0c`, `0x0013a1d4`, `0x0013a1e4` | Symbian AppView/AppUi/Document and runtime heap/exception thunks: **platform-specific/runtime/vendor**. | **X**. |
| High-address apparent functions `0x00166866`–`0x001ba69a` | **data/false function**, established in stage 16; false records were removed and are not SDK candidates. | **X**. |

## Handoff to sessions 30–34A

1. **Session 30 — complete:** scalar/conversion, Color4, vector, and quaternion rows are implemented in `Fixed.*`/`Color.hpp`, with native-derived `test_math.cpp` coverage and the completion ledger above. Existing consumers were not rerouted; repeated-x helpers and native truncation are preserved.
2. **Session 31:** implement matrix factories/inversion, project/unproject, Plane/frustum, and PointCloud with explicit ownership. Native allocation/copy bugs must be documented separately from any portable bounds/lifetime hardening.
3. **Session 32:** recover TextureTarget, sprite submission, 16-bit span fill, and dynamic-light ownership. Do not expose native scanline/runtime support as portable public API.
4. **Session 33:** recover the input/sound/dialog builders and ScreenRect/S3dPoint helpers. Keep original no-ops empty.
5. **Session 34:** rerun this inventory, add an external consumer linking only `soft3d`, and require every “missing useful API” row to become implemented or receive a documented exclusion with native evidence.
6. **Session 34A:** process `game_object_inventory.md` separately, recovering missing game-owned APIs and correcting the documented ActionUp edge without moving game policy into reusable Soft3D.

Ghidra carries corrected names/prototypes for the high-confidence Color4, Vec2/Vec3/Vec4, quaternion, projection, reflection, plane, and point-cloud bodies. `stage29`/`sdk-candidate` tags identify reviewed candidates. Session 30 added distinct signed/unsigned 16/17 reciprocal names, `Quaternion_Identity`, 47 cleaned prototypes, `stage30`/`sdk-recovered` tags on all 61 recovered/duplicate bodies, and eight bookmarks for native quirks; the program was saved and read back with no unsaved changes.
