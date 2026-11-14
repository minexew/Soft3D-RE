# SPL camera paths

## Identification and container

SPL contains a counted sequence of general degree-N Bezier camera-path records. `LoadSplCameraResource` (`0x00085ecc`) reads a FilePak entry, constructs a temporary curve, parses each record with `BezierCurve3_Parse` (`0x00084c30`), and appends a deep copy to a `CameraPathCollection`. Its open-error log identifies `BezierCurve3::LoadFromPak`.

```text
s32 recordCount
CameraPathRecord records[recordCount]
```

`Tools/outdir/common/1000.spl` is 1984 bytes: a four-byte count of 11 followed by eleven 180-byte cubic records. Scene setup allocates the collection with capacity 64 and loads this file immediately after `default.s3dsty`. `Scene_LoadMenuBackdrop` (`0x0002c268`) then loads `menu.sbp` and starts path playback.

## Serialized record layout

For `segmentCount = N`, the file stores `pointCount = N + 1`. The record size is `8 + 4 * (N*N + 11*N + 1)` bytes:

```c
struct SplCameraPathRecord {
    int32_t segmentCount;
    int32_t pointCount;
    float controlPoints[segmentCount + 1][3];
    float coefficientMatrix[segmentCount + 1][segmentCount + 1];
    float firstDifferences[segmentCount][3];
    float secondDifferences[segmentCount - 1][3];
};
```

The original parser converts every float to signed 16.16 fixed point by multiplying by 65536 and truncating. Its temporary runtime `BezierCurve3` is 24 bytes:

| Offset | Field |
|---:|---|
| `+0x00` | Control-point array pointer. |
| `+0x04` | `pointCount`. |
| `+0x08` | `segmentCount`, the degree. |
| `+0x0c` | Row-pointer array for a `pointCount × pointCount` fixed-point coefficient matrix. |
| `+0x10` | First-difference array, `segmentCount` vectors. |
| `+0x14` | Second-difference array, `segmentCount - 1` vectors. |

`CameraPathCollection_Append` (`0x00086220`) does not retain the serialized matrix or difference arrays. Its copy routine (`0x000883fc`) copies the `segmentCount + 1` knots and calls `BezierCurve3_Rebuild` (`0x00084380`). This matters for malformed or tool-generated files: the shipped cubic records contain NaNs in the unused upper triangle of the matrix, but those values are discarded before use. The portable loader likewise reads the knots and regenerates the rest rather than converting the NaNs.

The rebuilt matrix contains integer binomial coefficients in 16.16 form. For a cubic its rows are:

```text
1  0  0  0
1  1  0  0
1  2  1  0
1  3  3  1
```

The former `unknown78` and `unknown9c` fields are therefore confirmed as first and second differences:

```text
first[i]  = point[i + 1] - point[i]
second[i] = first[i + 1] - first[i]
```

`BezierCurve3_BinomialCoefficient` (`0x000849f8`) computes `degree! / ((degree-index)! * index!)` and shifts the integer quotient left 16.

## Curve evaluation

`BezierCurve3_Evaluate` (`0x000854e4`), `BezierCurve3_EvaluateDerivative` (`0x00085738`), and `BezierCurve3_EvaluateSecondDerivative` (`0x00085994`) use the same fixed-point recurrence. Time is clamped to `[0, 1]`. For control points `P`, degree `N`, and binomial matrix `C`:

```text
oneMinusTime = 1 - time
power = time
result = P[0] * oneMinusTime
for index = 1 .. N-1:
    coefficient = C[N][index] * power
    result = result + P[index] * coefficient
    result = result * oneMinusTime
    power = power * time
result = result + P[N] * power
result = result * scale
```

The scales are 1 for position, `N` for the first derivative, and `N * (N - 1)` for the second derivative. The derivative evaluators substitute the first- or second-difference arrays and use the corresponding matrix row. Every multiplication retains the low 32 bits after an arithmetic 16-bit right shift, including for negative values.

For the shipped first cubic at local time 0.5, the exact portable/reference results are:

```text
position          = (-4995310, -1787878, -2372946)
first derivative  = ( -987438,   353874,  1607376)
second derivative = ( 2184996,  -303948,  1818228)
```

## Collection ownership and timing

`CameraPathCollection_Ctor` (`0x00087d10`) allocates the requested number of 24-byte curve objects and initializes count and timing pointers to zero. The scene requests capacity 64. The runtime collection is 20 bytes:

| Offset | Field |
|---:|---|
| `+0x00` | Curve capacity. |
| `+0x04` | Curve count. |
| `+0x08` | Curve array. |
| `+0x0c` | Fixed-point control-polygon length per curve. |
| `+0x10` | Fixed-point cumulative normalized end time per curve. |

Every append, removal, or control-point edit rebuilds both timing arrays in `CameraPathCollection_RebuildTiming` (`0x00086288`). A path length is the sum of distances between adjacent **control points**, not integrated curve arc length. Vector length uses the original fixed-point square root with range reduction. If the sum of path lengths is `total`:

```text
pathEnd[i] = clamp(pathLength[i] / total + pathEnd[i - 1], 0, 1)
pathEnd[last] = 1
```

`CameraPathCollection_EvaluatePosition` (`0x0008689c`) and `EvaluateDerivative` (`0x00086ba0`) clamp global time, select the first path whose end time is at least the requested value, and convert to local time:

```text
local = time / pathEnd[0]                              for path 0
local = (time - pathEnd[i - 1]) / (pathEnd[i] - pathEnd[i - 1]) otherwise
```

Thus long control polygons receive proportionally more playback time. The real resource's control-polygon lengths sum to 14,714,880 fixed-point units. The eleven cumulative end times are:

```text
11070, 16326, 22537, 28230, 34085, 38814,
44816, 50521, 55546, 60586, 65536
```

The original also has editing helpers for removal, insertion/refinement, translation, transformation, and bounds. They are not needed by menu playback and are not part of the portable runtime API.

## Camera and `CameraSpot`

`Engine + 0x90` owns the 0x168-byte camera/projection helper constructed by `Camera_Ctor` (`0x000fc8c0`; duplicate `0x000fc238`). It owns the current position/target/up vectors, look-at matrix, perspective matrix, viewport, and forward/inverse projection scales.

`Camera_SetLookAt` (`0x0006dc60`) copies position, target, and up into the helper and builds the world-to-camera matrix. The basis is:

```text
forward = normalize(target - position)
right   = normalize(up × forward)
cameraUp = forward × right
translation = (-dot(right, position), -dot(cameraUp, position), -dot(forward, position))
```

Very small right-vector components are shifted left eight bits before normalization. The matrix stores right, up, and forward as successive rows, with final row `(0, 0, 0, 1)`.

The original defaults are angle `0x400`, near 1.0, and far 140.0. The screen descriptor stores `aspect = width / height`. The constructor and setter have a confirmed arithmetic inconsistency: `Camera_Ctor` computes the vertical angle as `trunc(angle * aspect)`, whereas `Camera_SetProjection` (`0x000fd138`) recomputes it as `trunc(angle / aspect)`. Both truncate to a 16-bit angle. With the original 320×240 descriptor, constructor/menu values are:

```text
aspect              = 87381
verticalAngle       = 1365
screenScaleX        = 18183604
screenScaleY        = 18986777
inverseScreenScaleX = 236
inverseScreenScaleY = 226
```

Calling `SetProjection` with those same defaults instead produces vertical angle 768, `screenScaleX = 34567141`, and `inverseScreenScaleX = 124`; the Y values are unchanged. The portable `Camera` deliberately preserves this constructor/setter difference.

Forward scales are `width / (2 * tan(verticalAngle / 2))` and `height / (2 * tan(angle / 2))`; inverse scales are the corresponding reciprocals in fixed point. The tangent table begins at `0x001a2084`, has 2049 entries, and uses the angle bit `0x800` for sign symmetry. Ordinary entries are truncated fixed-point tangents; the 45-degree entry is exactly `0x10000`, and the final entry is `0x7fffffff`.

The perspective matrix uses:

```text
scaleY = 1 / tan(angle / 2)
scaleX = scaleY / aspect
scaleZ = far / (far - near)
row 0 = (scaleX, 0, 0, 0)
row 1 = (0, scaleY, 0, 0)
row 2 = (0, 0, scaleZ, 1)
row 3 = (0, 0, -near * scaleZ, 0)
```

The default matrix entries are `scaleX = 118667`, `scaleY = 158223`, `scaleZ = 66007`, and translation Z `-66007`. The portable implementation reproduces the menu/default values exactly; non-default tangent entries currently come from the equivalent mathematical function rather than a copied 2049-word table.

RTTI identifies the generic `CameraSpot` interface and its `DummySpot` descendant. The interface has two destructor slots followed by position/right/up/forward getters and right/up/forward setters. `DummySpot` additionally exposes a position setter (`0x000226c8`). Its layout is:

| Offset | Field |
|---:|---|
| `+0x00` | Vtable pointer. |
| `+0x04` | Camera-controller back pointer. |
| `+0x08` | Position. |
| `+0x14` | Right vector. |
| `+0x20` | Up vector. |
| `+0x2c` | Forward vector. |

The portable `CameraSpot` and `DummySpot` classes implement this interface without pulling in `Player` or episode objects.

## Menu path playback and rendering

The larger game camera controller is not an engine `CameraSpot`; it consumes a spot and the SPL collection. Menu setup dispatches mode 5 through `CameraController_SetMode` (`0x0007b930`) to `CameraController_StartPath` (`0x0007cc44`) with speed 30.0, no loop, and no initial snap. `CameraController_UpdatePath` (`0x0007ec08`) is the active update callback.

For nonzero speed, each update performs:

```text
desiredPosition = paths.EvaluatePosition(time)
time += 1 / (16 * speed)
if time > 1:
    finished = true
    time = loop ? 0 : 1
desiredTarget = position + paths.EvaluateDerivative(time) * 10
position += (desiredPosition - position) * 0x1999
target = desiredTarget
camera.SetLookAt(position, target, (0, 1, 0))
```

`0x1999` is the original 0.1 approximation. At speed 30 the time increment is exactly 136, so the 482nd update crosses the endpoint. Position uses the pre-increment time while target uses the advanced time. Other gameplay camera modes, collision avoidance, shake, and trigger behavior are outside this menu-focused stage.

The reusable portable `MenuBackdrop` host owns `Bsp`, `CameraPathCollection`, `CameraPathController`, and `DummySpot`. `GameEngine::LoadMenuBackdrop` loads style/SPL/map resources and activates the host. On a menu frame, pre-update advances the camera; prepare-render builds a `BspRenderState` from the camera, prepares and rasterizes visible faces, and submits visible map models to the normal queues. No concrete enemy, trigger, episode, or `Scene` state-machine implementation is required.

## Portable verification

`Reversed_Game/src/soft3d/BezierCurve3.*`, `Camera.*`, `CameraSpot.*`, and `MenuBackdrop.*` implement the recovered path. `tests/test_spl.cpp` verifies:

- all 11 real records, fixed-point knots, binomial coefficients, differences, position/derivative samples, chord lengths, and cumulative times;
- synthetic linear evaluation, empty collections, malformed counts, inconsistent counts, truncation, trailing data, and deep-copy ownership;
- exact default projection values, look-at translation, all `DummySpot` accessors, 136-unit time steps, smoothing, endpoint clamping, and completion;
- 120 complete real-menu frames through `GameEngine` and dummy video, including camera movement, visible BSP faces, map-model submission, and presentation.

Exact raw/presented hashes are `0x57550571`/`0x9c578365` for the first menu frame, raw `0x5e0cf7d0` for the second, and `0xbda77efe`/`0x79f3684d` for frame 120. The older fixed-camera Stage 2 test now uses the recovered default perspective matrix for map models; its updated raw/presented hashes are `0x26bfdf7c` and `0x23fa83dd`.
