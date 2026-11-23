# Symbian 3rd full scene save/load and deterministic persistence

## Scope and implementation

Addresses in this document use the native-offset convention. Ghidra is saved at image base `0x8000`; add that offset for navigation (for example, native `Scene_LoadSave` `0x0002f864` is Ghidra `0x00037864`).

Game-completion stage 13 implements the complete `slot%d.sav` scene stream and `spt\\slot%d.sav` script sidecar in `Reversed_Game/src/game/SaveGame.cpp`. `Scene::SaveSaveSlot` and `Scene::LoadSaveSlot` are the production entry points used by the save menu and `GameEngine::LoadSaveSlot`; `MyGameUnitMgr::SaveState` and `LoadUnitState` own manager content and typed unit records. `LoadDlg` continues to read only the 44-byte header for display.

Typed stream integers and vectors are big-endian. Raw event blocks, manager timer/door blocks, header words, gameitem records, and waypoint payloads are little-endian. Wide strings use a big-endian signed 16-bit byte count followed by UTF-16 code units without a terminator. `OutputFileStream` writes its trailing sentinel, so a valid scene file ends in a zero sentinel byte; the sidecar does not use that stream format.

## Forty-four-byte slot header

The header is eleven little-endian 32-bit words. `GameEngine_SaveSaveSlot` (`0x000715d0`) fills it before `Scene_SaveSave` (`0x0002ff34`) writes the body.

| Offset | Contents |
|---:|---|
| `0x00` | Day. |
| `0x04` | Map. |
| `0x08` | Six little-endian 16-bit local timestamp fields: year, zero-based month, day, hour, minute, second. |
| `0x14` | Monotonically increasing save number, copied from a global counter after increment. This is what the slot UI draws; it is **not** play time. |
| `0x18` | Present flag, written as one. |
| `0x1c` | Item-use count. |
| `0x20` | Kill count. |
| `0x24` | Play time in seconds. |
| `0x28` | Death count. |

Loading a slot resets the player and restores the last four header statistics before the body is read. Saving snapshots the day-start item-use/kill counters, updates the header, and then writes the body. Portable `SaveSlotInfo::GetSaveNumber()` reads word 5 and `GetPlayTime()` reads word 9; the old `GetPlayTime()` mapping to word 5 was incorrect.

## Scene body order

`Scene_SaveSave` and `Scene_LoadSave` (`0x0002f864`) agree on the following order after the header:

1. Big-endian `int32 day` and `int32 map`.
2. Current music resource filename as a wide string, or an empty string.
3. For each of two environment channels: big-endian base delay, random delay, and resource count, followed by that many resource filenames. Elapsed time, next delay, and voice are transient; loading resets them to `0`, `-1`, and `0`.
4. The scene event block at native `0x1afc`, exactly `0x2800` raw bytes.
5. The day-start snapshot at native `0x42fc`, exactly `0x2800` raw bytes.
6. GameUI gun-HUD enable at native `GameUIDlg+0x4b0`, as one big-endian word. Other GameUI modal state is transient.
7. The complete `Player_SaveState` record.
8. `MyGameUnitMgr_SaveState` content and unit records, continuing until the stream sentinel/EOF.
9. The separately written script archive.

The reader loads day-specific GameUI/font resources, restores music/environment state, prepares concrete enemy pools before loading the player, and only then loads manager/unit state. It subsequently reloads the script sidecar and begins the normal restore-map transition. Map-owned camera and light triggers are excluded from the unit writer and reconstructed from BSP entity data. Restore finalization skips ordinary table-driven item/enemy spawning.

The portable implementation checks every fixed-section read, string, environment count, and sidecar result. Missing or truncated files and invalid days fail instead of continuing with partially read state. The `spt` directory is created for portable output.

## Manager content

The native manager writer (`0x00047cdc`) emits:

1. `0x100` raw bytes at manager `0x19c`: 32 two-word records. Both words initialize to `-1`; native update increments the second word when the first is zero. Their script-command semantics remain a stage-15 naming concern, so portable code preserves them as `serialized19c` rather than inventing a specific timer API.
2. `0x1000` raw bytes at manager `0x2500c`: 1024 little-endian door-state words.
3. Big-endian `int32` enemy-info group count, then each group as big-endian `int16 mapId` and `int16 recordCount`.
4. For each enemy spawn: two big-endian vectors, then big-endian `int16 active`, `enemyType`, `unitId`, and `waypointCount`, followed by raw little-endian 0x1c-byte waypoint records `{Vec3 position, int32 waitTime, Vec3 direction}`.
5. Big-endian `int32` gameitem group count, then big-endian `int16 mapId` and `int16 recordCount`, followed by raw little-endian 0x60-byte `GameItemSpawnRecord` records.
6. One hundred big-endian 32-bit item runtime-state words.
7. Unit records in bucket order 0 through 7, excluding map-owned buckets 6 and 7 and entries whose unit ID is `-1`.

The manager loader resets ordinary buckets and the serialized 32-record block, replaces both data tables transactionally after validation, resets the embedded StayBox cursor, and restores all content before unit reconstruction. Health-pack count belongs to the player record; manager reset must not clear it after the player has restored it.

Portable bounds checks limit enemy-info groups to the 64 embedded native groups and 32 records per group. The former portable capacity of 100 groups was incorrect: the native constructor initializes 64 0x484-byte groups. Dynamic gameitem groups and nested counts are checked against remaining input before allocation.

## Typed unit reconstruction

The original `MyGameUnitMgr_LoadUnitState` (`0x00048314`) consumes the big-endian type word first; each derived loader begins with unit ID. Its native switch covers:

| Type | Reconstructed object | Bucket/owner |
|---:|---|---|
| `0` | Base collision `GameUnit` | 1 |
| `4` | Generic `Things` | 2 |
| `0x1b` | `EventTrigger` | 4 |
| `0x1c` | `Breakable` | 2 |
| `0x1d` | `Pushable` | 2 |
| `0x1e` | `Pickable` | 2 |
| `0x1f` | `Trap` | 5 |
| `0x21` | `ActTrigger` | 4 |
| `0x22` | `DoorTrigger` | 4 |
| `0x23` | `HideTrigger` | 4 |
| `0x25` | `PickTrigger` | 4 |
| `0x26` | Next embedded StayBox | Manager array, not a pool |
| `0x27` | `ItemTrigger` | 4 |
| `0x28` | `TalkTrigger` | 4 |

PickTrigger appends a big-endian linked-thing ID after its own state. Zero means no link. A nonzero ID must resolve to a reconstructed Pickable in bucket 2; portable loading rejects an unresolved or wrongly typed link. Record `0x26` never represents an enemy.

Native writers can also emit live pooled-enemy records whose configured unit types are `0x0a`–`0x1a`, one for each of the 17 pool types. The native reconstruction switch has no cases for them. Portable stage 13 accepts these writer-generated records, parses the common enemy extension to identify its concrete enemy type, selects a free slot from that concrete pool, and replays the record into it. Spawn-table position/waypoint links are restored where a matching unit ID exists. No generic runtime enemy adapter is used.

The native manager writer does not iterate the embedded StayBox array even though its reader has case `0x26`. Portable writing emits active, non-`-1` StayBoxes after ordinary bucket records so stage-13 saves actually round-trip them. These enemy and StayBox additions are deliberate portable completion fixes for native writer/reader asymmetries, not claims that the original switch contains those cases.

## Determinism and portable corrections

Two native orderings prevent immediate second-save byte equality and are corrected deliberately in the portable implementation:

- Bucket insertion is head-first. Loading records with immediate insertion reverses every bucket. Portable reconstruction holds owned units and non-owning pooled-enemy references, then inserts each in reverse serialized order. This preserves list order and makes the next save identical.
- `Player_LoadState` resets the player, reapplies weapon resources, and resets the state machine after reading visual state. Those resets can clear serialized animation time. Portable code replays the saved visual record after the native reset sequence so the final object/animation state matches the bytes just loaded.

A save number and timestamp naturally change in a new header. Determinism therefore compares scene bytes from offset `0x2c` onward and compares the complete script sidecar. Environment scheduling cursors, voices, camera transition state, and other fields absent from the native schema are reset rather than serialized.

The portable `InputStream` additionally remembers read failures for checked scene reconstruction. This does not change its native cursor/sentinel layout or public result codes. Direct `InputDataStream` construction retains the legacy rule that the last supplied byte is a sentinel; `SetData` adds its own sentinel.

## Verification

`Reversed_Game/tests/test_game_save.cpp` covers:

- exact manager bytes and save → load → second-save equality;
- every native reconstruction type, linked pickup resolution, an embedded StayBox, and a concrete live enemy record;
- invalid types and linked-thing IDs, excessive/truncated manager sections, missing sidecars, and truncation across scene sections;
- header save-number/play-time mapping, both progression blocks, gun HUD, environment configuration, door state, and runtime item state;
- full scene and script-sidecar byte determinism from offset `0x2c` onward;
- resumed real script/map loading and subsequent engine frames with real player/BSP rendering.

All 37 default tests pass serially. Thirty SDL-independent tests also pass in each of the ASan/UBSan, host-math, STDIO-logging, and WAR-nibble builds. Build directories and test suites were run independently and serially.

## Ghidra writeback

Ghidra is saved at image base `0x8000`. Stage 13 adds `SaveSlotHeader` (0x2c) and `ManagerSerialized19cRecord` (0x08), changes `Scene+0x6afc` to `InputFileStream *pSaveStream`, names `Scene_SaveSave`, `GameEngine_SaveSaveSlot`, `GameEngine_LoadScriptArchive`, and `S3dTimer_GetLocalDateTimeWords`, and saves typed signatures/locals for ten persistence functions. Plate comments record section order, mixed endianness, header semantics, native enemy-tag and StayBox writer/reader asymmetries, and deterministic portable corrections. Ten `stage13` functions, two bookmarks, and a `Stage13Status` completion property are saved. Manager prototypes retain the useful `GameUnitManager*` view because applying the opaque derived layout degrades decompiler output.
