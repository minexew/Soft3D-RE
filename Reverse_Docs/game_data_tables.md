# 7 Days game-data tables and fixed resource names

## Scope and authority

This document covers the Symbian 3rd `itemdata.sdt`, `gameitem1.sdt`–`gameitem7.sdt`, and `einfo1.sdt`–`einfo5.sdt` tables, their ARM consumers, and the fixed animation/sound/door resource tables. The ARM binary is the behavioral authority; shipped files in `Reversed_Game/src/resources/res.spk` are fixtures. These SDT files are unrelated to the generic UI hierarchy in `sdt_format.md` and to the license records `message.sdt`/`domain.sdt`.

The portable implementation is `Reversed_Game/src/game/GameData.*`, `ResourceNames.cpp`, and `MyGameUnitMgr.*`, in the `seven_days_game` library. `tests/test_game_data.cpp` covers all shipped tables, malformed inputs, repeated reloads, and a real `GameEngine::RunFrame` integration path.

## `itemdata.sdt`

### Serialized layout

All fields are little-endian. The reference file is exactly 4,272 bytes:

```text
u32 itemCount                       // 67
ItemData items[itemCount]           // 52 bytes each
u32 textCount                       // 65
TextData texts[textCount]           // 12 bytes each
```

```c
struct ItemData {                   // 0x34 bytes
    int32_t itemId;                 // +0x00; runtime array index, 0..99
    int32_t nameString;             // +0x04
    char modelName[32];             // +0x08; narrow NUL-terminated name
    int32_t combineWithItemId;      // +0x28
    int32_t combinedItemId;         // +0x2c
    int32_t descriptionString;      // +0x30
};

struct TextData {                   // 0x0c bytes
    int32_t textId;                 // +0x00
    int32_t titleString;            // +0x04
    int32_t bodyString;             // +0x08
};
```

Text IDs 1000–1999 select `documents[textId - 1000]`; IDs 2000–2099 select `photos[textId - 2000]`. The original address arithmetic uses two differently based branches, but the resulting document/photo records are contiguous and this direct mapping is equivalent. There is no separate populated model-text section. About-model text and model names come from the item records.

Combination is directional. `Player_CombineItems` (`0x0003c940`) compares the first slot item's `combineWithItemId` with the second slot's item ID; on a match it consumes both slots and calls `Player_AddItem` for the first item's `combinedItemId`. For example, shipped items 17 and 18 name each other as combination inputs and both produce item 19.

### Runtime records and unlocks

`LoadItemDataSdt` (`0x00045d98`) writes 0x264-byte runtime item entries:

| Offset | Type | Meaning |
|---:|---|---|
| `+0x000` | `Object` (0x20c bytes) | Model renderer and resource state. |
| `+0x20c` | `wchar16[64]` | Model name converted from the narrow serialized field. |
| `+0x24c` | `int32_t` | Name string index. |
| `+0x250` | `int32_t` | Combination input item ID. |
| `+0x254` | `int32_t` | Combination result item ID. |
| `+0x258` | `int32_t` | Description string index. |
| `+0x25c` | `int32_t` | Saved runtime state, initially zero. |
| `+0x260` | `int32_t` | Model unlock flag, initially zero. |

The loader also configures the embedded `Object`, enables culling-disabled mode, and initializes the model-preview zoom field to fixed-point `0x3333`. `AboutDlg_PrepareModelRenderer` (`0x0004b78c`) indexes the same item array; a second model allocation must not be substituted for it.

Each document/photo runtime record is 12 bytes: title string, body string, and unlock word. The original constructor clears the unlock words. `Player_AddItem` (`0x0003c230`) sets the item's model unlock for IDs below 1001, the document unlock for note IDs 1001–2000, and the photo unlock above 2000. These correspond to the portable `GameConfiguration::unlocks` arrays. `GameContent` stores parsed content while `GameConfiguration` remains the sole owner of persistent unlock state.

`Player_HasItem` (`0x0003c4f4`) and `Player_RemoveItem` (`0x0003c624`) use nine ordinary inventory slots for IDs below 1001 and 128 note slots otherwise. `Player_CompactInventory` (`0x0003caf0`) preserves slot zero and reinserts the other occupied slots without sound or selection side effects.

The ARM loader's success return value is not reliably initialized, and its callers ignore it. The portable loader returns an explicit status instead of reproducing that undefined return value.

## `gameitem1.sdt`–`gameitem7.sdt`

### Serialized layout

```text
u32 groupCount
repeat groupCount times:
    s32 mapId                       // full 32-bit serialized word
    s32 recordCount                 // full 32-bit serialized word
    GameItemSpawn records[recordCount] // 0x60 bytes each
```

The group header is **eight bytes, not a packed four-byte `{int16 id, int16 count}` pair**. The original runtime retains only the low signed 16 bits of each header word.

```c
struct GameItemSpawn {              // 0x60 bytes
    int32_t itemId;                 // +0x00; zero suppresses spawning
    int32_t pickupSoundIndex;       // +0x04
    uint8_t packedModelName[16];    // +0x08; packed 5-bit name
    Vec3 position;                  // +0x18
    Vec3 axisX;                     // +0x24
    Vec3 axisY;                     // +0x30
    Vec3 axisZ;                     // +0x3c
    ObjectBounds pickupBounds;      // +0x48
};
```

The runtime group is eight bytes: `int16 mapId`, `int16 recordCount`, and a pointer to the owned record array. `ClearGameItemGroups` (`0x000465fc`) destroys every record array and the group array before reload/destruction.

`MyGameUnitMgr_SpawnMapItems` (`0x00047198`) selects the **first** group matching the current map. For every nonzero item ID it decodes the model name, creates a `Pickable` through `MyGameUnitMgr_AddThing` (`0x00044b74`), sets its item ID with `Pickable_SetItemId` (`0x0003f79c`), and creates the linked `PickTrigger` with the serialized sound and bounds. Unit IDs descend from 9999. Duplicate map groups are valid: day 4 contains map 3009 twice, and the first occurrence wins.

Shipped values:

| File | Bytes | Groups | Total spawn records |
|---|---:|---:|---:|
| `gameitem1.sdt` | 1228 | 9 | 12 |
| `gameitem2.sdt` | 2028 | 13 | 20 |
| `gameitem3.sdt` | 1452 | 13 | 14 |
| `gameitem4.sdt` | 2644 | 18 | 26 |
| `gameitem5.sdt` | 2324 | 14 | 23 |
| `gameitem6.sdt` | 612 | 4 | 6 |
| `gameitem7.sdt` | 412 | 3 | 4 |

## `einfo1.sdt`–`einfo5.sdt`

### Serialized layout

```text
u32 groupCount
repeat groupCount times:
    s32 mapId                       // full 32-bit serialized word
    s32 recordCount                 // full 32-bit serialized word
    repeat recordCount times:
        EnemySpawn spawn            // 0x20 bytes
        EnemyWaypoint waypoints[spawn.waypointCount] // 0x1c bytes each
```

```c
struct EnemySpawn {                 // 0x20 bytes
    int32_t enemyType;              // +0x00
    Vec3 position;                  // +0x04
    Vec3 direction;                 // +0x10
    int32_t waypointCount;          // +0x1c
};

struct EnemyWaypoint {              // 0x1c bytes
    Vec3 position;                  // +0x00
    int32_t waitTime;               // +0x0c
    Vec3 direction;                 // +0x10
};
```

Each runtime spawn is 0x24 bytes: position and direction, four signed 16-bit words (`active`, `enemyType`, `unitId`, `waypointCount`), and an owned waypoint pointer. Loading initializes `active = -1` and `unitId = -1`. Each group has a fixed 0x484-byte layout: two 16-bit header words and 32 runtime entries. `ClearEnemyInfoGroups` (`0x00046f6c`) destroys all nested arrays and resets the group count.

`MyGameUnitMgr_SpawnMapEnemies` (`0x00047470`) selects the current map's group, skips records whose `active` word is zero, chooses a stored unit ID when it is not `-1`, and otherwise allocates IDs downward from 9999. `MyGameUnitMgr_AddEnemy` (`0x000454a4`) obtains a pooled enemy and resolves spawn collisions. Every waypoint is appended through `Enemy_AddWaypoint` (`0x0007324c`) with its wait time and facing direction. A nonzero direction vector is applied to the enemy. `MyGameUnitMgr_UpdateEnemySpawnRecords` (`0x00047984`) synchronizes live state back to the table before changing maps.

Shipped values:

| File | Bytes | Groups | Spawn records | Notes |
|---|---:|---:|---:|---|
| `einfo1.sdt` | 4 | 0 | 0 | Valid empty day-1 file. |
| `einfo2.sdt` | 248 | 4 | 4 | Map 2001 has three waypoints. |
| `einfo3.sdt` | 428 | 9 | 11 | |
| `einfo4.sdt` | 44 | 1 | 1 | |
| `einfo5.sdt` | 1320 | 5 | 18 | Includes multiple waypoint routes. |

Days 6 and 7 legitimately have no `einfo` file. `MyGameUnitMgr_LoadDayContent` (`0x00043de4`) attempts `einfo` then `gameitem` and clears a separate 0x1000-byte progression block. Both original return values are ignored. The portable day loader accepts missing `einfo` only on days 6–7, requires valid `gameitem` on every day, and reports malformed/required-missing files explicitly.

## Fixed animation, sound, and door tables

`InitializeAnimationResourceNames` (`0x00089238`) fills **260** fixed 64-character UTF-16 slots at `GameEngine + 0xa8`, with 0x80-byte strides. It includes player/enemy animation names and deliberate duplicate and empty entries. Entries 238–259 are the 22 door/cinematic names.

`InitializeSoundResourceNames` (`0x00088594`, formerly `InitializeUiResourceNames`) fills **142** sound-name slots beginning at `GameEngine + 0x8328`. The old eight-name portable table was only indices 0–7. The recovered table includes all enemy/player sounds, door sounds, `a_getitem` at index 140, and `a_gunlevelup` at index 141, with original duplicates and empty entries preserved.

`Scene_InitializeDoorResources` (`0x0002b554`) independently initializes **22**, not 21, door records. Model entries match animation entries 238–259. Its two sound-index arrays are:

```text
open:  0,10,10,11,12,14,18,10,13,19,20,12,12,11,13,14,11,11,12,12,11,10
close: 0,15,15,16,17, 0, 0,15,15, 0, 0,17,17,16,15, 0,16,16,17,17,16,15
```

The portable `GameData` owns fixed 64-character tables for all three sets and installs the full sound-name table in `AudioManager` through `MyGameUnitMgr::Initialize`.

## Portable validation and integration

The portable parsers preserve valid-file order, fixed-point words, duplicate groups, inactive zero-item records, and runtime initial states. They additionally reject truncation, trailing data, negative/overflowing counts, invalid item/text/map/sound IDs, more than 100 `einfo` groups, and more than 32 spawns in one group. Parsing is transactional: malformed input does not partially replace existing content. These are malformed-input safety checks, not changes to shipped-data behavior.

`MyGameUnitMgr::Initialize` loads item data and binds the full sound table. The SDL application calls it after `GameEngine::Init`; its new-game hook selects day 1 through `MyGameUnitMgr::LoadDayContent`. `GameUIDlg` and `AboutDlg` continue to consume `GameContent` and `GameConfiguration`, so item descriptions, model names, document/photo text, and unlock arrays meet on the existing application path without duplicate persistent unlock ownership.
