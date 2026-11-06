# Symbian 3rd audio and software sound mixing

## Scope and reliability

This document covers `SymbianBin/out.bin` (`out.bin` in Ghidra), whose image base is zero. Function names below have been applied to that program. Names ending in `AudioManager` and `SoundInstance` describe behavior inferred from call graphs and field use; they are not RTTI names. `AudioData`, `s3dAudioDevice`, `CMixerThread`, `CMixerActive`, and `MMdaAudioOutputStreamCallback` do have RTTI records.

Several imported Symbian names in the current database are demonstrably inconsistent with their call sites (for example, allocation and memory-clearing calls appear as unrelated subsystem methods). Those imported labels must not be used as behavioral evidence. The local ARM code and data accesses described here were checked directly.

## RTTI and vtables

| Class | RTTI / inheritance | Vtable start | Recovered slots |
|---|---|---:|---|
| `AudioData` | `Resource` | `0x0015f948` | `AudioData_Dtor` `0x00114a30`, `AudioData_DeletingDtor` `0x00114ab0`, inherited `SetEngine`, `LoadSauResource` `0x00114b30` |
| `s3dAudioDevice` | unresolved external base | `0x0014d118` | `AudioDevice_Dtor` `0x000a4f68`, `AudioDevice_DeletingDtor` `0x000a501c` |
| `CMixerThread` | external base plus `MMdaAudioOutputStreamCallback` at `+0x04` | `0x0014d150`, secondary `0x0014d170` | destructors `0x000a3bbc`/`0x000a3ddc`; callbacks below |
| `CMixerActive` | unresolved external base | `0x0014d130` | destructors `0x000a62e8`/`0x000a6290`, `MixerActive_Start` `0x000a4c10`, active callback `0x000a4c88` |

Many vtable function words are odd while their target bytes are ARM instructions at the corresponding even address. These are not Thumb functions. The missing functions have been created at the even addresses without changing the vtable data.

The `CMixerThread` secondary-vtable callbacks adjust `this` by `-4` and forward to the main object:

| Address | Function | Target after adjustment |
|---:|---|---|
| `0x000a5958` | `MixerThread_MaoscBufferCopied` | `HandleStreamBufferCopied` `0x000a48e4` |
| `0x000a5960` | `MixerThread_MaoscOpenComplete` | `HandleStreamOpenComplete` `0x000a4978` |
| `0x000a5968` | `MixerThread_MaoscPlayError` | `HandleStreamPlayError` `0x000a4a4c` |

## SAU loading and decoding

`AudioData` is allocated as `0x134` bytes. In addition to its `Resource` base, the following fields are proven:

| Offset | Type | Meaning |
|---:|---|---|
| `+0x12c` | `uint32_t` | Decoded PCM byte count; assigned `sauByteCount * 2`. |
| `+0x130` | `int16_t *` | Decoded mono PCM buffer. |

`LoadSauResource(audioData, entryName, filePak)`:

1. Selects the entry in `s3dFilePak` and obtains its size by seeking to its end and calling `Tell`.
2. Reads the complete SAU payload into a temporary allocation.
3. Sets `pcmByteCount = sauByteCount * 2`.
4. Allocates and zeroes `pcmByteCount * 2` bytes, twice the decoded payload size.
5. Calls `DecodeSauAdpcm` and releases the compressed allocation.
6. Returns the FilePak selection error, or `4` on allocation failure, otherwise zero.

`DecodeSauAdpcm` at `0x00114ce0` is a headerless mono ADPCM decoder:

- four-bit codes are consumed from little-endian 32-bit words, eight codes per word;
- predictor and step index start at zero;
- the step table is at `0x00114ec8` and index-adjustment table at `0x00114ecc`;
- step index is clamped to `0..88` and predictor to signed 16-bit range;
- output is native little-endian signed 16-bit mono PCM.

Each compressed byte produces two PCM samples, hence the `sauByteCount * 2` decoded byte count. The larger zeroed allocation leaves a zero-filled region after the decoded samples. No sample rate or channel count is stored by this loader.

`GetAudioDataPcmSize` (`0x001145c8`) returns `+0x12c`; `GetAudioDataPcmBuffer` (`0x001145f0`) returns `+0x130`.

## High-level audio manager

The object constructed by `AudioManager_Ctor` (`0x00112fe0`) is `0x9c` bytes. It owns a pointer to the platform/mixer wrapper at `+0x04`, uses the resource manager at `+0x30`, keeps a dynamic sound-instance list at `+0x38`, a 16-pointer scheduling array at `+0x48`, and embeds `s3dLog` at `+0x88`.

Important entry points:

| Address | Function | Observed behavior |
|---:|---|---|
| `0x00113504` | `AudioManager_Update` | Updates sound instances, retires finished voices, reconciles the scheduling array, and advances music fades. |
| `0x00113820` | `AudioManager_PlayMusic` | Loads an SAU resource and starts it on voice 0; optionally begins a fade. |
| `0x00113950` | `AudioManager_PlaySoundByName` | Loads an SAU resource and allocates a free voice from 1 through 15. |
| `0x00113b84` | `AudioManager_PlaySoundData` | Starts an existing `AudioData` object on a free voice from 1 through 15. |
| `0x00113c68` | `AudioManager_StopMusic` | Stops voice 0 or starts a fade-out. |
| `0x00113cc4` | `AudioManager_StopAllSounds` | Stops all 16 voices and clears sound-instance scheduling state. |
| `0x00113d3c` | `AudioManager_SetGlobalVolume` | Sets the shared output attenuation; values below 2 disable music/effect levels. |
| `0x00113ddc` | `AudioManager_SetMusicVolume` | Clamps to `0..255` and updates voice 0. |
| `0x00113e5c` | `AudioManager_SetEffectsVolume` | Clamps to `0..255` and updates voices 1 through 15. |
| `0x00113f00` | `AudioManager_SetVoiceVolume` | Updates one voice when the shared level is greater than 1. |
| `0x00113f78` | `AudioManager_StartMusicFade` | Initializes fixed-point fade state and fade direction. |

The manager and destructors at `0x00113188`/`0x0011324c` release the platform wrapper and instance list. Engine frame processing calls `AudioManager_Update` through the engine's audio-manager pointer.

## Voice state and software mixing

The low-level voice-state block is embedded in `s3dAudioDevice`, beginning at device offset `+0x04`. `CMixerThread` keeps a pointer to this state at its own `+0x40`. All arrays below have 16 entries.

| State offset | Type | Meaning |
|---:|---|---|
| `+0x00` | `int32_t` | Shared output right-shift amount. |
| `+0x10` | `int16_t *[16]` | PCM source pointers. |
| `+0x50` | `uint32_t[16]` | Source end values supplied from `AudioData::pcmByteCount`. |
| `+0x90` | `uint32_t[16]` | Current sample positions. |
| `+0xd0` | `uint32_t[16]` | Direct volume or left-shift amount, depending on volume mode. |
| `+0x110` | `uint32_t[16]` | Loop flags. |
| `+0x150` | `uint32_t[16]` | Finished/free flags; 1 means available. |
| `+0x190` | `uint32_t[16]` | Volume modes; 1 selects direct `0..255` multiplication. |

Recovered operations:

| Address | Function | Behavior |
|---:|---|---|
| `0x000a5c04` | `StartAudioVoice` | Assigns source/end, clears position and finished flag, sets loop and initial volume. |
| `0x000a5da0` | `StopAudioVoice` | Clears source/end/position and marks the voice finished. |
| `0x000a5e78` | `IsAudioVoiceFinished` | Returns the finished/free flag. |
| `0x000a5eb8` | `SetAudioVoiceLooping` | Writes the loop flag. |
| `0x000a5f14` | `GetAudioVoiceLooping` | Reads the loop flag. |
| `0x000a5f54` | `SetAudioVoiceVolume` | Updates direct volume or shift according to volume mode. |
| `0x000a604c` | `GetAudioVoiceVolume` | Returns direct volume, or shift multiplied by 32. |
| `0x000a6174` | `SetAudioVoiceVolumeMode` | Selects direct versus shift volume representation. |
| `0x000a5154` | `GetAudioVoicePosition` | Returns the current sample position. |
| `0x000a60dc` | `SetAudioMasterVolume` | Converts a level to the shared shift representation. |
| `0x000a61d0` | `GetAudioMasterVolume` | Returns `1 << sharedShift`. |

`MixAudioVoices` (`0x000a4220`) is entirely integer software mixing:

1. Clears a 400-element signed 32-bit accumulation buffer (`0x640` bytes).
2. For each of 16 active voices, reads mono signed 16-bit samples and accumulates them.
3. In direct mode, clamps volume to `0..255` and adds `sample * volume`.
4. In shift mode, clamps the shift to `0..8` and adds `sample << shift`.
5. At the source end, either wraps to position zero for looping or clears the source and marks the voice finished.
6. Converts 400 accumulated samples to signed 16-bit output in an 800-byte buffer, applying the shared shift and saturating to `-32768..32767`.
7. Submits the output descriptor through the stream object at mixer-thread offset `+0x10`.

The mixer compares the sample position against the supplied PCM **byte count**, not that count divided by two. Because the loader allocates and zeroes twice the decoded byte count, this observed behavior reads the zero-filled tail rather than immediately exceeding the allocation. Preserve this behavior in any exact reconstruction.

No stereo panning is present in this mixing loop. Its output is a single mono sample sequence. Spatialization is currently explained by distance-dependent volume rather than left/right transforms.

## Positional sound

The `0x38`-byte sound-instance object constructed by `SoundInstance_Ctor` (`0x00116d04`) has these confirmed fields:

| Offset | Type | Meaning |
|---:|---|---|
| `+0x00` | pointer | Owning audio manager. |
| `+0x04` | 3 × fixed-point `int32_t` | Source position. |
| `+0x10` | `int32_t` | Calculated volume. |
| `+0x14` | `int32_t` | Assigned voice, or `-1`. |
| `+0x18` | 3 × fixed-point `int32_t` | Listener position. |
| `+0x24` | fixed-point `int32_t` | Cached distance. |
| `+0x28` | `uint32_t` | Loop flag. |
| `+0x2c` | `AudioData *` | Sound resource. |
| `+0x30` | `uint32_t` | Active/enable flag. |
| `+0x34` | `int32_t` | Index in the manager's instance list. |

`SetSoundInstancePosition` (`0x001170b0`) and `SetSoundInstanceListenerPosition` (`0x001171b8`) copy three fixed-point coordinates. `CalculateFixedPointDistance` (`0x00129824`) computes Euclidean distance using fixed-point arithmetic and square-root helpers.

`CalculateSpatialSoundVolume` (`0x00116fec`) performs:

```text
distance = Distance(listenerPosition, sourcePosition)
volume = 285 - FixedMultiply(distance, 0x38000) >> 16
volume = Clamp(volume, 0, 255)
```

`0x38000` is 3.5 in 16.16 fixed point. Therefore the attenuation is `285 - 3.5 * distance` in the engine's position units; it is **not** a division by 3.5. The result is sent to the assigned voice by `UpdateSoundInstanceVolume` (`0x00116e74`). `ActivateSoundInstance` (`0x00116ec4`) acquires a voice when necessary, and `StopSoundInstance` (`0x00116f94`) releases it.

## SDL replacement boundary

The future Linux implementation can map this design to SDL3 without emulating Symbian multimedia objects:

- keep SAU decoding and voice mixing in portable C++;
- mix a mono signed 16-bit stream, then use `SDL_AudioStream` for device-format conversion;
- use voice 0 for music/fades and voices 1–15 for effects;
- update positional volumes from the engine frame/update path;
- deliver completed PCM through `SDL_PutAudioStreamData` rather than `CMdaAudioOutputStream` callbacks.

The draft app currently provides only this SDL stream boundary and does not attach the recovered mixer.
