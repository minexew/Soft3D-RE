# Soft3D matrix, projection, ray, plane, and point-cloud APIs

## Scope and native types

Session 31 recovered the useful uncalled matrix/geometry surface identified by `soft3d_sdk_inventory.md`. Existing `BezierCurve3`, `CameraPathCollection`, `CameraPathController`, `CameraSpot`, and SPL storage/editing/bounds/timing implementations were rechecked against the inventory and required no additional API.

All arithmetic uses signed 16.16 fixed point with native wrapping and truncation. `Matrix4::values` retains the native storage order: a Vec3 transform computes rows `(0,4,8,12)`, `(1,5,9,13)`, and `(2,6,10,14)`. The public portable types are:

```cpp
struct Plane {
    Vec3 normal;
    Fixed distance;
};

struct Ray {
    Vec3 origin;
    Vec3 direction;
};

class PointCloud {
    std::vector<Vec3> points;
};
```

The corresponding native layouts are 16 bytes for `Plane`, 24 bytes for `Ray`, and an 8-byte count/pointer pair for `PointCloud`. Ghidra contains these structures and corrected prototypes. Plane distance is `dot(normal, point) + distance` (`0x00081298`). `PlaneFromPointNormal` (`0x0012ac38`) stores the supplied normal and `-dot(point, normal)`.

## Matrix operations

`Fixed.cpp` exposes native-order determinant and cofactor operations:

- `Determinant2x2` (`0x0011b804`) and the nine-scalar `Determinant3x3` (`0x0011b708`) are the minor primitives.
- `Determinant` (`0x0011b370`) expands the full 4x4 matrix in the native operation order.
- `Inverse` (`0x0011be64`, zero-initializing wrapper `0x001291c8`) writes all 16 cofactor/inverse-determinant products. A zero determinant produces an all-zero result through the recovered `FixedDivide(1,0)` behavior.
- `Inverse3x3` (`0x0011cc90`) is a separate operation, not a duplicate full inverse. It copies the input when `abs(determinant) < 0x28f`; otherwise it computes only the upper-left nine outputs. The portable value return deterministically zero-initializes the other seven entries instead of exposing uninitialized native output storage.
- `Transpose` also covers `0x0011bbac`, which is a transpose with in-place swaps, not the matrix-assignment function previously recorded in the inventory.

Factory and mutation pairs preserve native argument order and per-element rounding:

| Portable API | Native body/bodies | Contract |
|---|---|---|
| `TranslationMatrix` | `0x0011d698` | Identity with explicit XYZ translation. |
| `ScaleMatrix(x,y,z)` | `0x0011d3f0` | Diagonal scale factory. |
| `ScaleMatrix(matrix,x,y,z)` | `0x0011d2d4` | Scales entries 0/4/8/12 by X, 1/5/9/13 by Y, and 2/6/10/14 by Z, including the translation column. |
| `RotateX`, `RotateY`, `RotateZ` | `0x0011d8d4`, `0x0011de74`, `0x0011e408` | In-place single-axis multiplication with cached input pairs. |
| `RotateXMatrix`, `RotateYMatrix`, `RotateZMatrix` | reachable factories plus `0x0011e750` | Native table-driven entries; sine peak is `0xffff` in the faithful table configuration. |
| `EulerMatrix`, `RotateEuler` | `0x0011f2a8`, `0x0011e99c` | Three-angle XYZ coefficient set and in-place application. These are not axis-angle routines. |
| `AxisRotationMatrix`, `RotateAxis` | `0x0011ff7c`, `0x0011f6e0` | Axis-angle coefficients, including their native entry signs and rounding, followed by in-place application. |
| `AxesMatrix` | `0x000ec4a4` | Interleaves axis components in native storage order without normalization. |
| `PlaneProjectionMatrix` | `0x00123820` | `I - coefficients * normal` outer-product form, including translation from coefficient `w`; not a rotation-axis factory. |
| `ReflectionMatrix` | `0x00123bf4` | `I - 2*n*n^T`, translation `-2*d*n`. |
| `MatrixFromValues` | `0x00123f80` | Assigns 16 scalar arguments in increasing native index order. |
| `QuaternionToMatrix` | `0x0012c47c` | Native doubled-product quaternion conversion with homogeneous final row/column. |

`Multiply` retains its existing 64-bit product accumulation per output. The recovered in-place rotation functions instead perform individual fixed multiplies and adds, so they must not be replaced by `Multiply` when exact truncation matters.

## Projection, viewport, and homogeneous transforms

Several Session-29 “duplicate” labels were corrected after comparing every entry. The sign variants are observably different and remain separate portable APIs:

| Native address | Portable API | Exact convention |
|---|---|---|
| `0x00121738` | `OrthographicMatrix` | `2/width`, `2/height`, `1/(far-near)`, translation `-near*depth`. |
| `0x00121a08` | `OrthographicMatrixNegativeDepth` | `1/(near-far)`, translation `near*depth`. |
| `0x00121cc8` | `OrthographicBoundsMatrix` | Off-center orthographic XY terms with `-2` numerators and positive far-minus-near depth. |
| `0x00122040` | `OrthographicBoundsMatrixNegativeDepth` | Same bounded XY construction, distinct negative-depth terms. |
| `0x00122674` | `PerspectiveTanMatrix` | Reciprocal tangent half-angle, aspect division, `m[11]=-1`, `far/(near-far)`, translation `near*depth`. |
| `0x00122930` | `PerspectiveMatrix` | `2*near/width`, `2*near/height`, `far/(far-near)`, `m[11]=1`, translation `-near*depth`. |
| `0x00122bf0` | `PerspectiveMatrixNegativeDepth` | Distinct negative depth/w convention; not a compiled duplicate. |
| `0x00122ea4` | `FrustumMatrix` | Perspective left/right/bottom/top/near/far construction. This body was formerly misnamed `SetViewport`. |
| `0x00123208` | `FrustumMatrixNegativeDepth` | Distinct denominator and depth/w signs. |
| `0x00123564` | `ViewportMatrix` | Actual viewport mapping. Integer half-width/height truncate before fixed conversion; `m[10]=far-near` is not rescaled. |

Zero spans retain `FixedDivide`'s zero result. `TransformVec4` (`0x00127678`) computes all four homogeneous outputs, while `TransformInverse` (`0x00130414`) first applies the full native inverse.

`Project` (`0x00128f50`) converts the input to `(x,y,z,1)`, applies world/view/projection, divides XYZ by computed `w`, and then applies the affine viewport transform. `Unproject` (`0x0012907c`) applies affine inverse viewport/projection/view/world transforms in that order. It does **not** perform a final homogeneous divide; portable code preserves that limitation rather than advertising a general perspective round trip.

## Ray, plane, frustum, and slide operations

The functions at `0x00124118`–`0x001241bc` operate on `Ray`, not `Plane`:

- `NormalizeRay` normalizes direction and copies origin.
- `TransformRay` applies the same affine Vec3 transform to origin and direction. Direction therefore receives translation; this is not inverse-transpose or direction-only transformation.
- `DotDirection` dots the direction with a supplied vector.

`DistanceToRay` (`0x00129a0c`) assumes the native direction scaling, computes the perpendicular length, and optionally returns the projected point. If length is at most `0x28f`, the optional closest point is the original input point. `DistanceToLine` (`0x0012acb4`) intentionally applies another `FixedSqrt` to that already returned length.

`PlaneDistanceAndProject` (`0x00129ba4`) returns signed plane distance. Its optional projection copies the input when `distance <= 0x28f` without taking an absolute value; negative distances therefore take that copy path. `PlaneIntersectRay` (`0x00129f3c`) treats `abs(dot(normal,direction)) < 0x28f` as parallel and returns the origin's plane distance. Otherwise it writes `origin - direction * (planeDistance/denominator)` and returns zero; behind-origin intersections are not rejected. `SegmentIntersectionDistance` (`0x00129ce8`) preserves the native parallel test, plane construction, nested-root wrapper, and closest-point arithmetic.

`Slide` (`0x0012ab14`) returns `value - normal * dot(value,normal) * 0x1028f`. The `0x1028f` factor is preserved exactly. `OutsideFrustum` (`0x000cf8ec`) checks five planes in order and rejects when distance is strictly less than `-point.w`; equality is inside. Its point input is Vec4 storage, with XYZ used for distance.

## PointCloud ownership and algorithms

Portable `PointCloud` uses value ownership and deep copies. The native count/pointer algorithms map as follows:

- `FromPlane` (`0x0012c9d8`, duplicate `0x0012cdd0`) chooses a seed axis from the dominant absolute normal component, builds a tangent and cross-product bitangent, and emits four corners with extent `0x20000000`. The center is `normal * distance`, including the original positive-distance sign.
- `Allocate`, ordinary copy/assignment, and `Reversed` (`0x0012fefc`, `0x0012ffc8`, `0x0012d1c8`, `0x0012d304`) are represented by vector allocation, deep value copies, and reverse iteration.
- `GetPlane` (`0x0012d3bc`) uses the normalized cross product of the first two edges and `-dot(first,normal)`. `Area` (`0x0012d470`) sums half-length fan-triangle cross products.
- `Bounds` and `Center` (`0x0012d5cc`, `0x0012d648`) retain the invalid-bounds sentinel and fixed average. Empty portable centers return zero instead of performing native division by zero.
- `Optimize` (`0x0012d75c`) removes a vertex when the normalized incoming/outgoing dot is at least `0xffbe`. Portable ownership also fixes the native temporary-buffer leak when no vertex is removed.
- `Classify` (`0x0012da7c`) returns positive=1, negative=2, spanning=3, or on-plane=4 using strict epsilon comparisons.
- `ClipPositiveInPlace` (`0x0012dbd4`) and `ClippedPositive` (intended arithmetic of `0x0012e56c`) retain positive/on-plane vertices and insert edge intersections in native traversal order. Axis-normal components exactly equal to +/-1 use `-distance`/`distance` for the intersection coordinate; other components use the native fraction and interpolation.
- `Validate` (`0x0012fb18`) returns 0=valid, 1=too few points, 2=small area, 3=large coordinate, 4=short edge, 5=nonplanar, or 6=concave. Thresholds are area `1.0`, planarity `0x1999`, short-edge/convexity `0x28f`, and coordinate magnitude `0x20000000`, in the native check order.

The native in-place/allocating clipping bodies contain invalid count/allocation/copy lifetime behavior. The allocating body stores its allocated result only in a dead parameter slot and leaks it. Portable clipping implements the intended arithmetic with safe vector ownership and returns a value; these are intentional portable safety differences, not claims that the original routines returned usable clouds. The two-sided allocating variant `0x0012ef20` loses both outputs and leaks them, so it is intentionally omitted rather than assigned invented observable output semantics.

Empty `GetPlane`, `Optimize`, and clipping return a zero plane/no-op/empty cloud respectively instead of reproducing native out-of-bounds access or modulo-by-zero behavior. These guards affect only newly exposed unused APIs; reachable gameplay call sites were not rerouted.

## Verification

`test_math.cpp` checks exact determinant/inverse entries, singular behavior, identity and composition order, every rotation/factory family, all projection/sign variants, zero spans, odd viewport dimensions, reflection/projection construction, Vec4/inverse transforms, identity and affine project/unproject, zero `w`, and a directed perspective projection.

`test_geometry.cpp` checks plane/ray thresholds and optional outputs, behind/parallel rays, the nested-root and slide quirks, each frustum plane and boundary, every dominant plane-quad axis, deep/reversed ownership, allocation reuse, winding/area/bounds/center, empty/degenerate clouds, repeated optimization, all classifications, exact clip order and inside/outside/on-plane results, and every validation result 0 through 6.

Ghidra carries Stage 31 names, prototypes, `Plane`/`Ray`/`PointCloud` types, evidence comments, and `stage31`/`sdk-recovered` or `sdk-excluded` tags. The program was saved and read back with no unsaved changes.
