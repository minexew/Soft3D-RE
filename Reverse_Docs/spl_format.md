# SPL camera paths

## Identification and container

SPL contains camera-path records built around cubic Bezier curves. `LoadSplCameraResource` (`0x00085ecc`) reads a FilePak entry, obtains a record count, and appends each record to a camera-path collection. The parser log identifies `BezierCurve3::LoadFromPak`.

```text
u32 recordCount
CameraPathRecord records[recordCount]
```

`Tools/outdir/common/1000.spl` is 1984 bytes: a four-byte count of 11 followed by eleven 180-byte records. The menu/scene setup loads it immediately after `default.s3dsty` and before `menu.sbp` is incrementally loaded.

## Record layout recovered from `BezierCurve3_Parse` (`0x00084c30`)

```c
struct SplCameraPathRecord {
    int32_t segmentCount;
    int32_t pointCount;
    float controlPoints[4][3];
    float matrix[4][4];
    float unknown78[3][3];
    float unknown9c[2][3];
};                              // 180 bytes
```

The parser converts every float to signed 16.16 fixed point by multiplying by 65536 and truncating. It allocates:

- `pointCount` three-component control points;
- a `pointCount × pointCount` fixed-point matrix;
- `segmentCount` three-component records;
- `segmentCount - 1` three-component records.

The last two groups establish the byte layout, but their geometric meanings are not yet confirmed; the names `unknown78` and `unknown9c` are intentionally provisional. The curve object's constructor is `BezierCurve3_Ctor` (`0x00083940`), reset is `BezierCurve3_Reset` (`0x00084b10`), and the destructor is `BezierCurve3_Dtor` (`0x00084354`). `CameraPathCollection_Ctor` (`0x00087d10`) allocates capacity for 64 24-byte curve objects, and `CameraPathCollection_Append` (`0x00086220`) copies each parsed curve into the collection.

## Remaining work

Implementing the container alone is insufficient for the menu background. The camera stage also needs curve evaluation, path timing/interpolation, the engine camera/projection helper, and the connection to `CameraSpot`/scene rendering. Those routines should be validated against the 11 real records without decompiling episode-specific camera triggers.
