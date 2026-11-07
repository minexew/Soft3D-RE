# Symbian `ResManager` and resource-cache API

## Scope

This document covers `SymbianBin/out.bin` (`out.bin` in Ghidra), image base zero. RTTI names the class `ResManager`; the portable reconstruction calls it `s3dResManager` to match the neighboring Soft3D classes. The recovered types, function names, and signatures are applied in Ghidra.

## Class layout

`ResManager` inherits `s3dBase` and is allocated as **0x70 bytes** by `S3dEngine_ctor`.

| Offset | Size | Meaning |
|---:|---:|---|
| `+0x00` | 8 | `s3dBase`: vptr and engine/context pointer. |
| `+0x08` | 32 | Four `ResourceCache` objects, each 8 bytes. |
| `+0x28` | 72 | Embedded `s3dFilePak`. |

Each cache contains a 32-bit bucket count and a pointer to `bucketCount` 0x108-byte bucket records. The manager constructs four caches with **256 buckets** each. Only the bucket record's `next` field at `+0x104` is needed for a bucket head.

A cache node is also 0x108 bytes:

```c
struct ResourceCacheNode {
    uint32_t hash;             // +0x000
    char16_t key[64];          // +0x004, full path including extension
    Resource *resource;        // +0x100
    ResourceCacheNode *next;   // +0x104
};
```

The four caches are retention/cache levels, **not** resource-type arrays. Type and cache level are independent arguments.

## Manager entry points

| Address | Function | Behavior |
|---:|---|---|
| `0x000f3450` | `ResManager_Ctor` | Constructs the base, four 256-bucket caches, and the embedded FilePak. |
| `0x000f3bf4` | `ResManager_Dtor` | Clears all four caches, explicitly closes and destroys the FilePak, then destroys the caches and base. |
| `0x000f3a74` | `ResManager_DeletingDtor` | Deleting destructor. |
| `0x000e4e78` | `ResManager_GetResource` | Gets a cached resource or constructs, loads, and caches it. |
| `0x000e549c` | `ResManager_ReleaseResource` | Removes a resource from the first cache containing its path and deletes it; null is ignored. |
| `0x000e5390` | `ResManager_ClearCache` | Deletes every resource in one cache level and releases all its nodes. |
| `0x000317c8` | `GetFilePakFromManager` | Returns `manager + 0x28`. |
| `0x00071b5c` | `OpenGameResourceArchive` | Opens the embedded FilePak in streaming mode and discards the open status. |

The manager inherits `s3dBase::SetEngine`; it does not override that slot. `GetResource` passes the manager's engine pointer to each newly constructed resource.

## Name and directory construction

`ResManager_GetResource(manager, type, cacheLevel, name, directory)` returns null for a null or empty `name`. The type selects both extension and concrete class:

| Type | Extension | Concrete RTTI class | Allocation size |
|---:|---|---|---:|
| 0 | `.sau` | `AudioData` | `0x134` |
| 1 | `.soj` | `ObjectRes` | `0x15c` |
| 2 | `.sai` | `Animation` | `0x178` |
| 3 | `.stx` | `TextureRes` | `0x184` |

Directory handling is:

```text
null                    -> .\common\
starts with '.'         -> unchanged
starts with '\'         -> prefix '.'
anything else           -> prefix '.\'
non-null, no trailing '\' -> append '\'
```

The cache key is `normalizedDirectory + name + extension`. Comparisons use case-sensitive UTF-16 `wcscmp`.

## Cache lookup and promotion

All four levels are searched in ascending order. On a hit, the existing pointer is returned without checking its concrete type. If the requested level is lower than the level where it was found, the node is removed and the resource is inserted at the requested level. A higher requested level leaves the resource where it was.

The hash is unsigned 32-bit arithmetic followed by an unsigned bucket remainder:

```c
uint32_t hash = 0;
for each UTF-16 code unit c before NUL:
    hash = hash * 33 + c;
bucket = hash % bucketCount;
```

Nodes are appended to the bucket chain. Insertion rejects duplicate keys and returns zero; `GetResource` nevertheless returns the resource pointer without checking insertion success.

On a miss, the manager constructs the requested class, sets its engine, initializes its paths, and invokes the virtual loader through the embedded FilePak. Loader success returns zero; the resource is then cached. A loader error deletes the resource and returns null. Invalid types return null.

## Cache implementation helpers

| Address | Function |
|---:|---|
| `0x000f385c` | `ResourceCache_Ctor` |
| `0x000f3908` | `ResourceCache_Dtor` |
| `0x000e5584` | `ResourceCache_Find` |
| `0x000e570c` | `ResourceCache_Insert` |
| `0x000e5640` | `ResourceCache_Remove` |
| `0x000e5b18` | `ResourceCache_ClearNodes` |
| `0x000e5bd0` | `ResourceCache_Hash` |
| `0x000e5c5c` | `ResourceCache_DeleteNode` |

`ResManager_ClearCache` first gathers resource pointers using the small dynamic-pointer-array helpers at `0x000e5868` through `0x000e5d54`, deletes each through its deleting destructor, and only then frees the cache nodes.

## Portable reconstruction

The implementation is in `Reversed_Game/src/soft3d/s3dResManager.*`, with `Resource`, `AudioData`, `ObjectRes`, `Animation`, `TextureRes`, and `Texture` in neighboring files. The C++ version uses portable ownership containers while preserving lookup order, path construction, promotion, deletion, loader status handling, and known parser quirks.

`Reversed_Game/tests/test_s3dresmanager.cpp` exercises synthetic instances of all four formats, hash-bucket operation, cache hits and promotion, directory normalization, release and clearing, and real resources from `src/resources/res.spk`. Resource formats and exact parser behavior are documented in `resource_parsing_api.md`.
