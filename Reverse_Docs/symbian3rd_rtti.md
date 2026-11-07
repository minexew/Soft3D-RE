# Symbian 3rd binary RTTI

## Scope and conclusions

This document covers `SymbianBin/out.bin`, also opened in Ghidra as `/7days.bin` (`out.bin`). All addresses are Ghidra memory addresses in the loaded image; the file offsets from the E32Image import inventory have already been rebased by `0x8000` where applicable.

The binary contains Itanium/ARM-EABI-style C++ RTTI for its own engine and game classes, not merely references to an external C++ runtime. A total of 133 local type-info records were recovered:

- 17 `__class_type_info` records for classes without RTTI base links;
- 109 `__si_class_type_info` records for single inheritance;
- 7 `__vmi_class_type_info` records for multiple inheritance;
- 133 vtable structures referencing 125 of the 133 type-info records.

The inheritance graph is acyclic. Every nonzero base offset in a multiple-inheritance record has a matching secondary vtable with the corresponding negative offset-to-top. All 133 demangled class names are unique.

## Runtime support and direct use

The import table contains the following RTTI-related facilities:

- `__dynamic_cast`, with its import veneer at `0x00139cf4`;
- `std::type_info`, `std::bad_cast`, and `std::bad_typeid` methods and RTTI objects;
- `__cxxabiv1` class, single-inheritance, multiple-inheritance, pointer, function, and other type-info implementations;
- exception support including `__cxa_throw`, `__cxa_begin_catch`, `__cxa_end_catch`, `__cxa_bad_cast`, and `__cxa_bad_typeid`;
- 65 imported type-info objects: 10 `__cxxabiv1` runtime types, 17 fundamental types, 17 pointer types, 17 const-pointer types, three standard exception/type-info types, and `XLeaveException`;
- 15 imported vtables associated with runtime type-info classes, standard exception/type-info classes, `XLeaveException`, and `CCommsDbConnectionPrefTableView`.

There is a direct `__dynamic_cast` call at `0x000a4140` in `FUN_000a4090`. Its result is stored at object offset `+0x10` and then used through a virtual call. This demonstrates active RTTI use rather than unused runtime linkage.

## Binary layouts

All fields are 32-bit little-endian unless stated otherwise.

```c
struct RTTI_ClassTypeInfo {             // 0x08 bytes
    void *vtable;                       // +0x00
    char *name;                         // +0x04, Itanium-encoded type name
};

struct RTTI_SIClassTypeInfo {           // 0x0c bytes
    RTTI_ClassTypeInfo type_info;       // +0x00
    RTTI_ClassTypeInfo *base_class;     // +0x08
};

struct RTTI_VMIClassTypeInfo {          // 0x10-byte header
    RTTI_ClassTypeInfo type_info;       // +0x00
    uint32_t flags;                     // +0x08
    uint32_t base_count;                // +0x0c
    RTTI_VMIClassBase bases[];          // +0x10
};

struct RTTI_VMIClassBase {              // 0x08 bytes
    RTTI_ClassTypeInfo *base_class;     // +0x00
    uint32_t offset_flags;              // +0x04; object offset = value >> 8
};
```

The raw runtime type-info vtable marker values are `0x0008007b` for no-base records, `0x0008007f` for single-inheritance records, and `0x00080080` for multiple-inheritance records. They are useful for locating records but are not treated as local vtable structures themselves.

A vtable begins with an eight-byte RTTI header. Its address point, stored in an object's vptr, is the first function slot at header offset `+0x08`:

```c
struct VtableHeader {
    int32_t offset_to_top;              // +0x00
    RTTI_ClassTypeInfo *type_info;      // +0x04
    void *first_virtual;                // +0x08, vtable address point
};
```

For example, the `s3dFilePak` vtable starts at `0x00146618`, while its address point is `0x00146620`. This reconciles the terminology with `Reverse_Docs/filepak_api.md`.

## Ghidra reconstruction

The following artifacts have been applied to the Ghidra program:

- 133 `rtti_name_*` labels on encoded type-name strings;
- 133 `typeinfo_*` labels on local type-info records;
- 133 `vtable_*` labels on vtable starts and 133 `vtable_*_functions*` labels on their address points;
- seven `rtti_bases_*` labels on multiple-inheritance base arrays;
- typed `RTTI_ClassTypeInfo`, `RTTI_SIClassTypeInfo`, and `RTTI_VMIClassTypeInfo` records;
- typed `RTTI_VMIClassBase` arrays and typed vtable headers, producing data references from vtables to type-info records;
- 133 partial class structures named `RTTI_<ClassName>`.

Class structures contain only fields supported by RTTI:

- a `vptr` where a local or derived vtable proves a polymorphic subobject;
- `base_<Class>` fields for locally known bases;
- `base_external_<address>` fields using `RTTI_ExternalBase` for unresolved external bases.

Single-inheritance bases are at object offset zero. Multiple-inheritance base offsets come from `offset_flags >> 8` and are independently confirmed by secondary-vtable offset-to-top values. For example, `RTTI_Player` has `base_Creature` at `0x0`, `base_StateMachine_Player` at `0x6f0`, and `base_CameraSpot` at `0x704`.

RTTI does **not** reveal ordinary member variables, their names, or the total class size. Most Ghidra structure sizes are therefore only minimum known extents. `RTTI_State_Player`, `RTTI_XLeaveException`, `RTTI_MEikStatusPaneObserver`, and `RTTI_MAknTouchPaneObserver` contain a one-byte `layout_unknown` placeholder because no field layout is currently proven; the placeholder does not assert a real size of one byte.

Subsequent constructor analysis has fully recovered the layouts of `s3dBase`, `s3dIO`, `s3dLog`, `s3dFilePak`, `Resource`, and `ResManager`; these are exceptions to the partial-layout rule. `s3dBase` is 8 bytes (vptr plus engine/context pointer), `s3dIO` is 12 bytes (base plus platform-file pointer), `s3dLog` is 20 bytes (base plus embedded `s3dIO`), and `s3dFilePak` is `0x48` bytes. `Resource` is `0x12c` bytes and `ResManager` is `0x70` bytes, including four cache objects and an embedded FilePak. See `filepak_api.md`, `resource_parsing_api.md`, and `resmanager_api.md` for complete layouts and evidence.

Unresolved external base references occur in `CIMEI`, `CMixerActive`, `CMixerThread`, `CPhoneObserver`, `CSmsSessionObserver`, `CSocketReader`, `CSocketWriter`, `CSymbian3rdApplication`, `CSymbian3rdAppUi`, `CSymbian3rdAppView`, `CSymbian3rdDocument`, `s3dAudioDevice`, `SessionDevice`, and `VID_impl`.

Eight classes have no local vtable found by this scan: `StateSet<Player>`, `State<Player>`, `XLeaveException`, `MMdaAudioOutputStreamCallback`, `MAbortDirectScreenAccess`, `MDirectScreenAccess`, `MEikStatusPaneObserver`, and `MAknTouchPaneObserver`. Some are nevertheless proven polymorphic through derived-class vtables.

## Complete local RTTI inventory

Vtable addresses in this table are structure starts; add eight bytes for the address point. Numbers in parentheses after vtable addresses are signed decimal offset-to-top values.

| Class | Type-info | Name string | Kind | Bases (object offset) | Vtable starts (offset-to-top) |
|---|---:|---:|---|---|---|
| AboutDlg | `0x0013e5a4` | `0x0013e598` | single | Dialog @ `0x0` | `0x0013e508` (0) |
| ActTrigger | `0x00145578` | `0x00145568` | single | GameUnit @ `0x0` | `0x00145530` (0) |
| Animation | `0x0015fce4` | `0x0015fcd8` | single | Resource @ `0x0` | `0x0015fcc0` (0) |
| AudioData | `0x0015f96c` | `0x0015f960` | single | Resource @ `0x0` | `0x0015f948` (0) |
| BasicShader | `0x0015b454` | `0x0015b444` | class | — | `0x0015b430` (0) |
| BillboardGroup | `0x0015f6f0` | `0x0015f6dc` | single | Renderable @ `0x0` | `0x0015f6c8` (0) |
| BlindShader | `0x0015c5e0` | `0x0015c5d0` | single | BasicShader @ `0x0` | `0x0015c4c0` (0) |
| Boss | `0x001426c0` | `0x001426b8` | single | Enemy @ `0x0` | `0x001425e0` (0) |
| Breakable | `0x001424e0` | `0x001424d4` | single | Things @ `0x0` | `0x00142470` (0) |
| Bsp | `0x0015f5b4` | `0x0015f5ac` | multiple | Renderable @ `0x0`, s3dBase @ `0x18` | `0x0015f580` (0), `0x0015f598` (-24) |
| BspUnit | `0x0015e058` | `0x0015e04c` | single | GameUnit @ `0x0` | `0x0015e010` (0) |
| Bully | `0x00142358` | `0x00142350` | single | Enemy @ `0x0` | `0x00142280` (0) |
| BullyDay3 | `0x001457b4` | `0x001457a8` | single | Enemy @ `0x0` | `0x001456d8` (0) |
| BullyDay5 | `0x0013c43c` | `0x0013c430` | single | Enemy @ `0x0` | `0x0013c2f0` (0) |
| CameraSpot | `0x0013c194` | `0x0013c184` | class | — | `0x0013c158` (0) |
| CameraTrigger | `0x00142038` | `0x00142028` | single | GameUnit @ `0x0` | `0x00141ff0` (0) |
| CIMEI | `0x0014c950` | `0x0014c948` | single | external `0x76d` @ `0x0` | `0x0014c928` (0) |
| ClientSession | `0x001467b0` | `0x001467a0` | class | — | `0x00146788` (0) |
| CMixerActive | `0x0014d1c4` | `0x0014d1b4` | single | external `0x76d` @ `0x0` | `0x0014d130` (0) |
| CMixerThread | `0x0014d194` | `0x0014d184` | multiple | external `0x854` @ `0x0`, MMdaAudioOutputStreamCallback @ `0x4` | `0x0014d150` (0), `0x0014d170` (-4) |
| CPhoneObserver | `0x0015981c` | `0x00159808` | single | external `0x76d` @ `0x0` | `0x001597e8` (0) |
| Creature | `0x00141edc` | `0x00141ed0` | single | PhysicUnit @ `0x0` | `0x00141e58` (0) |
| CSmsSessionObserver | `0x00159840` | `0x00159828` | single | external `0x76d` @ `0x0` | `0x001597c8` (0) |
| CSocketReader | `0x001467e4` | `0x001467d4` | single | external `0x76d` @ `0x0` | `0x00146748` (0) |
| CSocketWriter | `0x001467c8` | `0x001467b8` | single | external `0x76d` @ `0x0` | `0x00146768` (0) |
| CSymbian3rdApplication | `0x001a4aa4` | `0x001a4a88` | single | external `0xd80` @ `0x0` | `0x001a4a38` (0) |
| CSymbian3rdAppUi | `0x001a5dc0` | `0x001a5dac` | single | external `0xe28` @ `0x0` | `0x001a5c58` (0) |
| CSymbian3rdAppView | `0x001a5280` | `0x001a5268` | single | external `0x169` @ `0x0` | `0x001a5198` (0) |
| CSymbian3rdDocument | `0x001abee0` | `0x001abec8` | single | external `0xd6c` @ `0x0` | `0x001abe58` (0) |
| DarkKen | `0x0013c02c` | `0x0013c020` | single | Enemy @ `0x0` | `0x0013bf50` (0) |
| Dialog | `0x0015de28` | `0x0015de20` | class | — | `0x0015de00` (0) |
| DialogManager | `0x0015dbe8` | `0x0015dbd8` | multiple | Renderable @ `0x0`, s3dBase @ `0x18` | `0x0015dba8` (0), `0x0015dbc4` (-24) |
| DoorTrigger | `0x00141d58` | `0x00141d48` | single | GameUnit @ `0x0` | `0x00141d10` (0) |
| DummySpot | `0x0013c1a8` | `0x0013c19c` | single | CameraSpot @ `0x0` | `0x0013c128` (0) |
| ElectricSaw | `0x0013c424` | `0x0013c414` | single | VisUnit @ `0x0` | `0x0013c3c0` (0) |
| Enemy | `0x00141c10` | `0x00141c08` | single | Creature @ `0x0` | `0x00141b38` (0) |
| Engine | `0x0015d7f8` | `0x0015d7f0` | single | s3dBase @ `0x0` | `0x0015d7a8` (0) |
| EventTrigger | `0x00141a20` | `0x00141a10` | single | GameUnit @ `0x0` | `0x001419d8` (0) |
| FadeShader | `0x0015c598` | `0x0015c588` | single | BasicShader @ `0x0` | `0x0015c508` (0) |
| FakeShadow | `0x0015d5f4` | `0x0015d5e4` | single | Renderable @ `0x0` | `0x0015d5d0` (0) |
| FatMaid | `0x001460ec` | `0x001460e0` | single | Enemy @ `0x0` | `0x00146010` (0) |
| FogShader | `0x0015b468` | `0x0015b45c` | single | BasicShader @ `0x0` | `0x0015b418` (0) |
| Font | `0x0015c2b8` | `0x0015c2b0` | single | s3dBase @ `0x0` | `0x0015c268` (0) |
| GameEngine | `0x001418d8` | `0x001418c8` | single | Engine @ `0x0` | `0x00141880` (0) |
| GameSI | `0x001413ec` | `0x001413e4` | single | ScriptInterface @ `0x0` | `0x001413d0` (0) |
| GameUIDlg | `0x0013f48c` | `0x0013f480` | single | Dialog @ `0x0` | `0x0013f460` (0) |
| GameUnit | `0x0013c6d0` | `0x0013c6f4` | single | MsgReceiverInterface @ `0x0` | `0x0013c678` (0) |
| GameUnitManager | `0x0015d4d0` | `0x0015d4bc` | single | s3dBase @ `0x0` | `0x0015d490` (0) |
| Ghost | `0x00145ae8` | `0x00145ae0` | single | Enemy @ `0x0` | `0x00145a10` (0) |
| GrayShader | `0x0015c57c` | `0x0015c56c` | single | BasicShader @ `0x0` | `0x0015c520` (0) |
| GunFire | `0x0013dc20` | `0x0013dc14` | single | Renderable @ `0x0` | `0x0013dc00` (0) |
| HelpDlg | `0x0013e5bc` | `0x0013e5b0` | single | Dialog @ `0x0` | `0x0013e4e8` (0) |
| HideTrigger | `0x0013ef50` | `0x0013ef40` | single | GameUnit @ `0x0` | `0x0013ef08` (0) |
| HttpSession | `0x0014681c` | `0x0014680c` | single | SessionDevice @ `0x0` | `0x001466d8` (0) |
| InkShader | `0x0015c5b0` | `0x0015c5a4` | multiple | BasicShader @ `0x0`, s3dBase @ `0x14` | `0x0015c4d8` (0), `0x0015c4f0` (-20) |
| InputDataStream | `0x0013afb8` | `0x0013b01c` | single | InputStream @ `0x0` | `0x0013af00` (0) |
| InputFileStream | `0x00160030` | `0x0016003c` | single | InputDataStream @ `0x0` | `0x00160020` (0) |
| InputStream | `0x0013afac` | `0x0013b00c` | single | Stream @ `0x0` | `0x0013af10` (0) |
| ItemTrigger | `0x001458f8` | `0x001458e8` | single | GameUnit @ `0x0` | `0x001458b0` (0) |
| Kate | `0x0013bcd0` | `0x0013bcc8` | single | Enemy @ `0x0` | `0x0013bbf8` (0) |
| Lamper | `0x00145cd8` | `0x00145cd0` | single | Enemy @ `0x0` | `0x00145c00` (0) |
| Lensflare | `0x0015d2a0` | `0x0015d294` | single | Renderable @ `0x0` | `0x0015d280` (0) |
| LightTrigger | `0x0013ebd8` | `0x0013ebc8` | single | GameUnit @ `0x0` | `0x0013eb90` (0) |
| LoadDlg | `0x0013e58c` | `0x0013e580` | single | Dialog @ `0x0` | `0x0013e528` (0) |
| Lord | `0x0013bae0` | `0x0013bad8` | single | Enemy @ `0x0` | `0x0013ba08` (0) |
| MAbortDirectScreenAccess | `0x00153630` | `0x00153644` | class | — | — |
| Maid | `0x0013e978` | `0x0013e970` | single | Enemy @ `0x0` | `0x0013e8a0` (0) |
| Maid2 | `0x0013e98c` | `0x0013e984` | single | Enemy @ `0x0` | `0x0013e7d0` (0) |
| Maid3 | `0x0013e9a0` | `0x0013e998` | single | Enemy @ `0x0` | `0x0013e700` (0) |
| MainUIDlg | `0x0013e5ec` | `0x0013e5e0` | single | Dialog @ `0x0` | `0x0013e4a8` (0) |
| MAknTouchPaneObserver | `0x001abe18` | `0x001abe3c` | class | — | — |
| MDirectScreenAccess | `0x00153638` | `0x00153660` | single | MAbortDirectScreenAccess @ `0x0` | — |
| MEikStatusPaneObserver | `0x001abe10` | `0x001abe20` | class | — | — |
| MMdaAudioOutputStreamCallback | `0x0014dc28` | `0x0014dc30` | class | — | — |
| MotionBlurShader | `0x0015c560` | `0x0015c54c` | single | BasicShader @ `0x0` | `0x0015c538` (0) |
| MsgReceiver | `0x0015ce78` | `0x0015ce84` | single | MsgReceiverInterface @ `0x0` | `0x0015cd70` (0) |
| MsgReceiverInterface | `0x0013c6c8` | `0x0013c6dc` | class | — | `0x0013c6b0` (0) |
| MyClientSession | `0x0013ade4` | `0x0013add0` | single | ClientSession @ `0x0` | `0x0013adb8` (0) |
| MyGameUnitMgr | `0x0013e2cc` | `0x0013e2bc` | single | GameUnitManager @ `0x0` | `0x0013e290` (0) |
| Object | `0x0015cc74` | `0x0015cc6c` | single | Renderable @ `0x0` | `0x0015cc58` (0) |
| ObjectRes | `0x0015cb54` | `0x0015cb48` | single | Resource @ `0x0` | `0x0015cb30` (0) |
| OptionDlg | `0x0013e574` | `0x0013e568` | single | Dialog @ `0x0` | `0x0013e548` (0) |
| OutputDataStream | `0x0013afd0` | `0x0013b040` | single | OutputStream @ `0x0` | `0x0013aee0` (0) |
| OutputFileStream | `0x0015ff24` | `0x0015ff10` | single | OutputDataStream @ `0x0` | `0x0015fee0` (0) |
| OutputStream | `0x0013afc4` | `0x0013b030` | single | Stream @ `0x0` | `0x0013aef0` (0) |
| PhysicUnit | `0x0015c9c0` | `0x0015c9b0` | single | VisUnit @ `0x0` | `0x0015c950` (0) |
| Pickable | `0x0013df50` | `0x0013df44` | single | Things @ `0x0` | `0x0013dee0` (0) |
| PickTrigger | `0x0013ddc8` | `0x0013ddb8` | single | GameUnit @ `0x0` | `0x0013dd80` (0) |
| PlayDlg | `0x0013e5d4` | `0x0013e5c8` | single | Dialog @ `0x0` | `0x0013e4c8` (0) |
| Player | `0x0013dc34` | `0x0013dc2c` | multiple | Creature @ `0x0`, StateMachine<Player> @ `0x6f0`, CameraSpot @ `0x704` | `0x0013db28` (0), `0x0013dbc0` (-1776), `0x0013dbd0` (-1796) |
| PlayerStateSet | `0x0013d8ec` | `0x0013d8d8` | single | StateSet<Player> @ `0x0` | `0x0013d310` (0) |
| Prisoner | `0x0013b754` | `0x0013b748` | single | Enemy @ `0x0` | `0x0013b678` (0) |
| Pushable | `0x0013d210` | `0x0013d204` | single | Things @ `0x0` | `0x0013d1a0` (0) |
| Raster | `0x0015b98c` | `0x0015b984` | single | s3dBase @ `0x0` | `0x0015b970` (0) |
| Renderable | `0x0013c898` | `0x0013c8a0` | class | — | `0x0013c880` (0) |
| Renderer | `0x0015b620` | `0x0015b614` | single | s3dBase @ `0x0` | `0x0015b600` (0) |
| ResManager | `0x0015d97c` | `0x0015d9b0` | single | s3dBase @ `0x0` | `0x0015d8f8` (0) |
| Resource | `0x0013af94` | `0x0013aff0` | single | s3dBase @ `0x0` | `0x0013af38` (0) |
| s3dAudioDevice | `0x0014d1e4` | `0x0014d1d0` | single | external `0x854` @ `0x0` | `0x0014d118` (0) |
| s3dBase | `0x0013af80` | `0x0013afdc` | class | — | `0x0013af68` (0) |
| s3dExit | `0x0015d970` | `0x0015d9a4` | single | s3dBase @ `0x0` | `0x0015d910` (0) |
| s3dFilePak | `0x0014663c` | `0x0014662c` | single | s3dBase @ `0x0` | `0x00146618` (0) |
| s3dImageLoader | `0x00146600` | `0x001465ec` | single | s3dBase @ `0x0` | `0x001465d8` (0) |
| s3dIO | `0x0013af88` | `0x0013afe8` | single | s3dBase @ `0x0` | `0x0013af50` (0) |
| s3dLog | `0x001464c4` | `0x001464bc` | single | s3dBase @ `0x0` | `0x001464a8` (0) |
| s3dMemInfo | `0x0015d958` | `0x0015d988` | single | s3dBase @ `0x0` | `0x0015d940` (0) |
| s3dTimer | `0x0013e3e0` | `0x0013e3ec` | single | s3dBase @ `0x0` | `0x0013e3c8` (0) |
| s3dVideo | `0x0015d964` | `0x0015d998` | single | s3dBase @ `0x0` | `0x0015d928` (0) |
| Sam | `0x0013b560` | `0x0013b558` | single | Enemy @ `0x0` | `0x0013b488` (0) |
| Scene | `0x0013d01c` | `0x0013d014` | single | s3dBase @ `0x0` | `0x0013d000` (0) |
| ScriptInterface | `0x001a4198` | `0x001a4184` | class | — | `0x001a4170` (0) |
| SessionDevice | `0x00146800` | `0x001467f0` | single | external `0x76d` @ `0x0` | `0x00146710` (0) |
| SimpleScript | `0x001a434c` | `0x001a433c` | single | s3dBase @ `0x0` | `0x001a4328` (0) |
| State<Player> | `0x0013d158` | `0x0013d174` | class | — | — |
| StateMachine<Player> | `0x0013dd60` | `0x0013dd68` | class | — | `0x0013dd50` (0) |
| StateSet<Player> | `0x0013d150` | `0x0013d160` | class | — | — |
| StayBox | `0x0013c8f4` | `0x0013c8e8` | single | GameUnit @ `0x0` | `0x0013c8b0` (0) |
| Stream | `0x0015ff08` | `0x0015ff00` | class | — | `0x0015fef0` (0) |
| StringManager | `0x0015c2a4` | `0x0015c294` | single | s3dBase @ `0x0` | `0x0015c280` (0) |
| Sword | `0x0013c770` | `0x0013c768` | multiple | GameUnit @ `0x0`, Renderable @ `0x54` | `0x0013c718` (0), `0x0013c754` (-84) |
| TalkTrigger | `0x00146230` | `0x00146220` | single | GameUnit @ `0x0` | `0x001461e8` (0) |
| TCPSession | `0x00146838` | `0x00146828` | single | SessionDevice @ `0x0` | `0x001466a0` (0) |
| Texture | `0x0015bcf0` | `0x0015bce4` | single | s3dBase @ `0x0` | `0x0015bcd0` (0) |
| TextureRes | `0x0013afa0` | `0x0013affc` | single | Resource @ `0x0` | `0x0013af20` (0) |
| Things | `0x0015bfe4` | `0x0015bfdc` | single | PhysicUnit @ `0x0` | `0x0015bf78` (0) |
| Trap | `0x0013c578` | `0x0013c570` | single | GameUnit @ `0x0` | `0x0013c538` (0) |
| TwistShader | `0x0015c5fc` | `0x0015c5ec` | single | BasicShader @ `0x0` | `0x0015c4a8` (0) |
| Victor | `0x0013b370` | `0x0013b368` | single | Enemy @ `0x0` | `0x0013b298` (0) |
| VID_impl | `0x0014dcd4` | `0x0014dcc8` | multiple | external `0x76d` @ `0x0`, MDirectScreenAccess @ `0x1c` | `0x0014dc90` (0), `0x0014dcb8` (-28) |
| VisUnit | `0x0015bd88` | `0x0015bd7c` | single | GameUnit @ `0x0` | `0x0015bd28` (0) |
| Weed | `0x00145ec8` | `0x00145ec0` | single | Enemy @ `0x0` | `0x00145df0` (0) |
| Worm | `0x0013b180` | `0x0013b178` | single | Enemy @ `0x0` | `0x0013b0a8` (0) |
| XLeaveException | `0x0014c8f0` | `0x0014c8f8` | class | — | — |

## Audio/video follow-up

The RTTI-guided audio and video passes are documented in `symbian3rd_audio.md` and `symbian3rd_video.md`. Their vtable targets have been defined and named in Ghidra, including the previously missing ARM functions for `AudioData`, `s3dAudioDevice`, `CMixerThread`, `CMixerActive`, `VID_impl`, `Raster`, `Texture`, and `Lensflare`. Odd vtable values in these records point to ARM code at the corresponding even addresses; they are not evidence of Thumb mode.

Recovered behavior now includes SAU decoding, the 16-voice mono software mixer, positional distance attenuation, the `VID_impl` lifecycle, CPU texture allocation, triangle rasterization, texture blitting, and the frame presentation path. The documents distinguish concrete RTTI names from behavioral names assigned to the audio manager and sound-instance objects. Resource follow-up analysis also recovered the `ResManager` cache API and the SAU/SOJ/SAI/STX parsing paths; see `resmanager_api.md` and `resource_parsing_api.md`.

## Recommended next recovery steps

Use constructor and destructor writes to establish actual class sizes and ordinary fields, then extend the partial `RTTI_*` structures. Vtable function slots can be recovered systematically from each address point, but slot names still require behavioral analysis or leaked-source correlation. Applying the reconstructed class types to known `this` parameters will make those later passes substantially clearer. For video, the highest-value remaining link is the writer and concrete type of the presenter pointer at `Raster + 0x1a7c`.
