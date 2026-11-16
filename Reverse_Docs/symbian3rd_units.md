# Symbian3rd generic units, messaging, states, and physics

## Scope and reconstruction status

Stage 11 covers the reusable infrastructure, not episode-specific `Scene`, `Player`, `Enemy`, trigger, or game-data behavior. `Reversed_Game/src/soft3d/Units.*`, `StateMachine.hpp`, and `BspCollision.cpp` implement it, with directed tests in `Reversed_Game/tests/test_units.cpp`.

The RTTI hierarchy is:

```text
MsgReceiverInterface
├── MsgReceiver
└── GameUnit
    └── VisUnit
        └── PhysicUnit
            ├── Things
            └── Creature
```

`GameUnitManager` derives from `s3dBase`. `StateMachine<Player>` and `CameraSpot` are separate `Player` bases at `0x6f0` and `0x704`; they are not `Creature` bases.

Ghidra now has function names, prototypes, comments, stage tags, and native structure layouts for this subsystem. Confirmed sizes are `GameUnit` `0x54`, `VisUnit` `0x90`, `PhysicUnit` `0x668`, `Things` `0x684`, `Creature` `0x6f0`, `GameUnitManager` `0x194`, and `PlayerStateSet` `0x4f4`. The public BSP trace result is 48 bytes. Two ordinary alternate `GameUnitManager` constructors were recovered at `0x000ef53c` and `0x000ef6d0`; the latter had been missed by function analysis.

## Messages

`MsgReceiverInterface` has two destructor slots followed by `GetMsgReceiver` and `ProcessMessages`. The defaults at `0x000119c8` and `0x000119ac` return null and do nothing.

`MsgReceiver` is `0xf8` bytes: a vptr, 20 three-word messages, and a 32-bit count at `0xf4`. Its constructor only clears the count. `MsgReceiver_AddMessage` (`0x000ecc8c`) appends `{type, value, data}` without a capacity check. `MsgReceiverInterface_SendMessage` (`0x000ece68`) calls virtual slot 2 and appends only when it returns a queue. The portable implementation ignores messages beyond capacity 20 instead of overwriting the object.

The routines at `0x000ece38` and `0x000eced0` belong to the scene's separate receiver-pointer array, not to the embedded 20-message `MsgReceiver` queue.

## `GameUnit` and manager

The `GameUnit` constructor (`0x00026ba4`) sets type 0, id -1, zero position/local/collision bounds, no bucket link, visible = true, and collision enabled = true.

Important native fields are:

| Offset | Meaning |
|---:|---|
| `0x04` | Unit type |
| `0x08` | Unit id |
| `0x0c` | Position |
| `0x18` | Local min/max bounds |
| `0x30` | Collision min/max bounds |
| `0x48` | Next unit in manager bucket |
| `0x4c` | Visible/submission flag |
| `0x50` | Collision enabled |

The three query methods return `this` on a hit and null otherwise. Point and AABB tests are inclusive on all six comparisons (`0x00026914`, `0x0002682c`). Segment/AABB testing (`0x0002689c`) uses the center and half extents of the box, half the segment direction, and the three face and three cross-product separating axes. Save/load slots are present but their stream implementation belongs to stage 12.

`GameUnitManager` has eight `{count, first}` buckets at `0x38`, a result count at `0x78`, 64 result pointers at `0x7c`, and six host context pointers at `0x17c`–`0x190`.

- `GameUnitManager_AddUnit` (`0x00049860`) links at the bucket head. It does not set the unit's manager back-pointer.
- Removal supports identity and id, with optional deleting destruction (`0x000ef95c`, `0x000efa74`).
- `ResetBuckets` (`0x000ef8dc`) only zeros counts/heads; `ClearBucket` (`0x000efb90`) can destroy the old chain.
- Update, post-update, and submission visit all eight buckets. Update saves `next` before virtual dispatch so a unit may remove itself.
- `FindUnit` searches only the requested bucket.
- Bucket `-1` queries visit only buckets 0 through 2. Point and segment queries then require `visible`; bounds queries do not check `visible`. Specific-bucket queries do not check `visible`. This asymmetry is original behavior.

The original result writer has no count bound. The portable result is capped at 64 hits.

## `VisUnit`

`VisUnit` sets type 1 and owns an object group, optional alternate shadow object or `FakeShadow`, animation state, resource/renderer/audio pointers, and an optional positional `SoundInstance`.

`ObjectGroup` stores count, current LOD, camera depth, objects, and renderer. LOD update (`0x000ee0a8`) starts at 70 fixed-point units and halves the threshold while moving toward object zero. Submission selects opaque queue 3 with negated depth or transparent queue 4 with positive depth. A shadow is submitted in queue 2 only when its transform is dirty, then the dirty flag clears.

`VisUnit_LoadObjects` (`0x000d9374`) creates the group and selects object zero. The multi-entry loader constructs LOD names but passes the original name to `Object_Load`; those constructed names have no effect. Portable groups use vectors rather than reproducing the original fixed-allocation overflow risk.

`SetDirection` keeps existing axes for `abs(x) < 0x28f && abs(z) < 0x28f`; otherwise it flattens Y, normalizes forward, and constructs right/up/forward axes. Axis setters copy the vector and recompute its length. Animation is applied to every group object and the optional shadow object, with time and completion reset. `PhysicUnit_SetAnimation` additionally copies secondary bounds.

Direct one-shot sounds return voice -1. Positional sound uses `SoundInstance` and removal by its list index, not its mixer voice. `VisUnit_StopSound` (`0x000da37c`) preserves that distinction.

## `PhysicUnit`

The constructor (`0x000e5f10`) sets type 2, flags `0xf`, friction `0x8000`, zero velocity/yaw, and empty traces.

Flag meanings confirmed from `PhysicUnit_Update` (`0x000e64b8`) are:

| Bit | Behavior |
|---:|---|
| 0 | Subtract friction from Y velocity when not on ground |
| 1 | Use the collision movement branch; clear integrates velocity directly |
| 2 | Select simple rather than slide movement |
| 3 | Enable step/ground handling |
| 4 | Apply the `0x4000` step-up offset |
| 5–7 | Restore previous X/Y/Z after movement |
| 8 | Keep the current direction instead of deriving it from velocity |
| 9 | Test Y separately during unit collision and include Y in zero-velocity checks |

Update order is significant: clear axis-collision flags; save previous position; apply friction; update direction/yaw; move or integrate; restore selected axes; perform step/ground traces; update visuals and LOD; sample BSP light; update shadow; translate collision bounds. The light value minus `0x4ccc` is written to **object intensity at `Object + 0x5c`**, not depth bias at `+0x60`.

`MoveSimple` (`0x000e74b4`) rejects a unit-overlap result, otherwise performs one BSP trace. `MoveSlide` (`0x000e6b78`) first tests X/Z and optionally Y against the unit manager, zeroes blocked components, then performs at most five BSP slide iterations. It projects velocity with multiplier `0x1028f` (approximately 1.01), restores original speed, and multiplies the remaining scale by the hit fraction exactly as in ARM.

Ground checks trace down `0x8000`. Shadow tracing runs from +`0x20000` to -`0x140000` in Y. Its matrix is `I - direction * plane`, where direction is `(0,1,0)` for `FakeShadow` or a normalized header-derived XZ vector for shadow objects.

After movement, `GameUnit.collisionBounds = localBounds + position`, while `secondaryCollisionBounds = secondaryLocalBounds + position`. These are separate assignments.

## `Things`

`Things` sets type 4 and physics flags `0x1e0`. Destruction disables all three collision queries. Normal rendering uses the visual base only for nonzero material-0 blend or a matching cached leaf cluster; destruction renders the alternate object.

Destruction (`0x000dbf3c`) stores a signed 16-bit duration and resets the timer. At duration-6, -4, and -2 updates, material 0 blend becomes 3, 2, and 1. At the exact duration the unit removes itself from bucket 2 with deletion. The signed thresholds are preserved, including their behavior for short durations. `Things_Update` also performs initial ground placement and synchronizes the alternate model transform.

Alternate model/animation loaders return zero even when the underlying load fails; null-resource safety checks in the portable implementation are explicit rather than reproducing a crash. `MoveTowards` (`0x000dbf90`) compares squared distance, snaps on arrival, and otherwise takes one normalized fixed-point step.

## `Creature`

`Creature` sets type 6, navigation speed `0x8000`, and empty path/hearing state. `NavigateTo` (`0x00076c94`) does the following:

1. Snaps and returns true inside the horizontal radius.
2. Uses direct movement when the target trace has `startSolid == 0`.
3. Retries after adding the trace plane normal, but still navigates toward the original target if that succeeds.
4. Otherwise finds visible nearest start/target path nodes, tracks previous/current nodes, and follows a path-pair link.

The path-pair selection preserves the original cumulative `nodes[1]` comparison against `targetNode.index`; it is not replaced with an invented A* search.

`HearSound` (`0x00076aec`) distinguishes position-based and id-based events and clears stale state outside range. `SetNavigationEnabled` clears destination state and velocity when enabled. `SnapToGround` traces down `0xa0000` and updates previous position.

## State templates

`State<Player>` is 12 bytes: vptr, enabled byte, and next-state pointer. Its five slots include enter, exit, update, and shared registration; registration stores the state in the set and enables it.

`StateMachine<Player>` stores current, owned previous, owned pending, and the state set. Current states are borrowed from the set. `StateMachine_Player_Reset` exits the old chain, obtains the initial state, and enters the new chain. Enter is child/next-first; exit is outer-first. Transition polling is also child-first and uses the first non-null update result. After a transition, any pending state is deleted.

`PlayerStateSet` has 256 registration pointers (1024 bytes) followed by 20 embedded 12-byte states, giving `4 + 0x400 + 20 * 12 = 0x4f4` bytes. Registration does not transfer ownership.

The portable templates reproduce these rules without implementing any concrete player state.

## BSP collision and path queries

`BspTraceResult` layout:

| Offset | Meaning |
|---:|---|
| `0x00` | End position |
| `0x0c` | Start-solid flag |
| `0x10` | Plane normal and distance |
| `0x20` | Brush start-solid flag |
| `0x24` | All-solid flag |
| `0x28` | Fraction |
| `0x2c` | Brush side index |

`Bsp_Trace` (`0x00011378`) dispatches equal endpoints to point-solid traversal and other endpoints to swept-bounds tracing. The equal-endpoint path unusually classifies `start + end`; this is retained. Swept tracing projects local bounds onto node planes, uses a `0x50000` margin for non-axis planes, splits at fixed-point enter/leave fractions, and clips solid leaf brushes once per trace. A successful hit adds plane normal * `0x51e` to the interpolated endpoint.

Trace reciprocals use the same table as the renderer: `0x40000000 / index`, with index zero returning `0x7fffffff`, followed by range-dependent shifts. Portable arithmetic does not substitute an exact floating-point division.

`Bsp_FindNearestPathNode` (`0x00106dd8`) selects the nearest nonnegative squared-distance node whose trace has `startSolid == 0`; `0xffff` is the null path-node index.

`Bsp_GetLightIntensity` (`0x000fe2cc`) traverses to a lower lightmapped face, applies additional style frames, adds positive point-light contributions, clamps, and scales. The generic unit implementation writes that value to object intensity as described above.

### Leaf traversal correction

`Bsp_FindLeaf` (`0x001070c8`) selects serialized child 0 when plane distance is greater than zero and child 1 otherwise. `Bsp_LoadNodes` preserves serialized child order, so the previous portable condition was reversed.

With the fixed camera `(-66,-22,-43)`, corrected traversal reports cluster -1, no decompressed PVS stream, and 160 visible faces rather than cluster 0, five PVS bytes, and 166 faces. No queue-3 map models are submitted in that view. Raw/presented hashes remain `0x00841c73`/`0x832d70f5`; the menu SPL first/second/final hashes are also unchanged.

## Verification

The unit suite uses synthetic message receivers, manager units, state chains, a six-plane solid brush, physics units, and creature helpers. It checks virtual dispatch, message capacity, all three query types, bucket ownership/removal, update ordering, transition order, swept collision and solid flags, movement, destruction, hearing, and navigation.

Real-resource tests load `menu.sbp` without an episode and exercise leaf/point/swept queries, and load `cg_bully_tuomao_a` twice through `VisUnit`, materials, transforms, direction, shadow creation, and submission.

All 25 normal CTest tests and all 19 SDL-independent ASan/UBSan tests pass. All six standalone SOJ converter tests also pass. Ghidra has been saved after verifying typed decompilation of `PhysicUnit_Update` and `Bsp_ClipBrush`.
