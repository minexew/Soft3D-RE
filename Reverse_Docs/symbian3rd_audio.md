# Symbian 3rd audio and software sound mixing

## Scope and reliability

This document covers `SymbianBin/out.bin` (`out.bin` in Ghidra), whose image base is zero. Function names below have been applied to that program. Names ending in `AudioManager`, `AudioPlayer`, and `SoundInstance` describe behavior inferred from call graphs and field use; they are not RTTI names. `AudioData`, `s3dAudioDevice`, `CMixerThread`, `CMixerActive`, and `MMdaAudioOutputStreamCallback` do have RTTI records.

Ghidra has `Audio`/`SoftwareMixer` function tags, concrete `AudioVoiceState`, `s3dAudioDevice`, `AudioPlayer`, `AudioManager`, `SoundInstance`, `SoundInstanceVector`, and `CMixerThread` structure types, and `AudioRecovery`, `AudioVoiceStateLayout`, `AudioManagerLayout`, `SoundInstanceLayout`, and `AudioThreadLayout` property maps containing the recovered facts and offsets. The principal forwarding routines, manager methods, thread operations, parameters, and decompiler locals have also been renamed or typed. Voice-control prototypes use the device base; `CMixerThread::state` points to the embedded state at device `+0x04`.

## RTTI and vtables

| Class | RTTI / inheritance | Vtable start | Recovered slots |
|---|---|---:|---|
| `AudioData` | `Resource` | `0x0015f948` | `AudioData_Dtor` `0x00114a30`, `AudioData_DeletingDtor` `0x00114ab0`, inherited `SetEngine`, `LoadSauResource` `0x00114b30` |
| `s3dAudioDevice` | external `CBase` | `0x0014d118` | `AudioDevice_Dtor` `0x000a4f68`, `AudioDevice_DeletingDtor` `0x000a501c` |
| `CMixerThread` | external base plus `MMdaAudioOutputStreamCallback` at `+0x04` | `0x0014d150`, secondary `0x0014d170` | destructors `0x000a3bbc`/`0x000a3ddc`; callbacks below |
| `CMixerActive` | external `CActive` | `0x0014d130` | destructors `0x000a62e8`/`0x000a6290`, `MixerActive_Start` `0x000a4c10`, `MixerActive_RunL` `0x000a4c88` |

Many vtable function words are odd while their target bytes are ARM instructions at the corresponding even address. These are not Thumb functions. The missing functions were created at the even addresses without changing the vtable data.

The `CMixerThread` secondary-vtable callbacks adjust `this` by `-4` and forward to the main object:

| Address | Function | Target after adjustment |
|---:|---|---|
| `0x000a5958` | `MixerThread_MaoscBufferCopied` | `HandleStreamBufferCopied` `0x000a48e4` |
| `0x000a5960` | `MixerThread_MaoscOpenComplete` | `HandleStreamOpenComplete` `0x000a4978` |
| `0x000a5968` | `MixerThread_MaoscPlayError` | `HandleStreamPlayError` `0x000a4a4c` |

## SAU loading and decoding

`AudioData` is allocated as `0x134` bytes. In addition to its `Resource` base:

| Offset | Type | Meaning |
|---:|---|---|
| `+0x12c` | `uint32_t` | Decoded PCM sample count; assigned `sauByteCount * 2`. |
| `+0x130` | `int16_t *` | Decoded mono PCM buffer. |

`LoadSauResource(audioData, entryName, filePak)`:

1. Selects the entry in `s3dFilePak` and obtains its size by seeking to its end and calling `Tell`.
2. Reads the complete SAU payload into a temporary allocation.
3. Sets `pcmSampleCount = sauByteCount * 2`.
4. Allocates and zeroes `pcmSampleCount * 2` bytes, exactly two bytes per decoded sample.
5. Calls `DecodeSauAdpcm` and releases the compressed allocation.
6. Returns the FilePak selection error, or `4` on allocation failure, otherwise zero.

`DecodeSauAdpcm` at `0x00114ce0` is a headerless mono ADPCM decoder:

- four-bit codes are consumed from little-endian 32-bit words, eight codes per word;
- predictor and step index start at zero;
- the step table is at `0x00114ec8` and index-adjustment table at `0x00114ecc`;
- step index is clamped to `0..88` and predictor to signed 16-bit range;
- output is native little-endian signed 16-bit mono PCM.

Each compressed byte produces two PCM samples, so the stored value is a sample count rather than a byte count. The allocation is exactly large enough for the decoded samples; there is no extra decoded-data tail. No sample rate or channel count is stored by this loader.

`GetAudioDataPcmSampleCount` (`0x001145c8`) returns `+0x12c`; `GetAudioDataPcmBuffer` (`0x001145f0`) returns `+0x130`.

The S2D SDK's `AudioData::DecoderVer` uses the same step/index tables and predictor algorithm, confirming shared ancestry. It consumes the high nibble first, whereas the examined Soft3D SAU build consumes the low nibble first. WAR is the same format with high-nibble-first order; because each runtime has hardcoded order and no runtime swap, this was probably selected by a compile-time option rather than representing a Soft2D/Soft3D format change. S2D's eight-channel `AudioPlayer` also expands into the 16-voice Soft3D mixer. See `s2dsdk_correlation.md`.

The reconstruction defaults to SAU order. Configure with `-DSOFT3D_AUDIO_REVERSE_NIBBLE_ORDER=ON` to build its decoder for WAR order; the option is compile-time and adds no runtime format detection.

## High-level audio manager

The object constructed by `AudioManager_Ctor` (`0x00112fe0`) is `0x9c` bytes. Its confirmed fields are:

| Offset | Type | Meaning |
|---:|---|---|
| `+0x00` | pointer | Optional table of 64-UTF-16-unit sound names used by indexed overloads. |
| `+0x04` | `AudioPlayer *` | Four-byte platform-player wrapper. |
| `+0x08`, `+0x0c` | `int32_t` | Music and effect levels. |
| `+0x10`, `+0x14` | fixed `int32_t` | Current music-fade level and per-update increment. |
| `+0x18`, `+0x1c` | `int32_t` | Remaining and originally requested fade updates. |
| `+0x20` | `int32_t` | Nonzero for fade-in, zero for fade-out. |
| `+0x24` | 12 bytes | Zero-initialized; not assigned a speculative meaning. |
| `+0x30` | `s3dResManager *` | Resource owner. |
| `+0x34` | `AudioData *` | Current music resource, non-owning. |
| `+0x38` | 16-byte vector | Dynamic `SoundInstance *` list. |
| `+0x48` | `SoundInstance *[16]` | Per-update positional scheduling array. |
| `+0x88` | `s3dLog` | Embedded logger. |

Primary entry points:

| Address | Function | Observed behavior |
|---:|---|---|
| `0x00113504` | `AudioManager_Update` | Runs device recovery, updates/retires instances, rebuilds the schedule, and advances music fades. |
| `0x00113820` | `AudioManager_PlayMusic` | Loads an SAU at cache level 3 and starts it on voice 0; optionally begins a fade. |
| `0x00113950` | `AudioManager_PlaySoundByName` | Loads an SAU and allocates the first free voice from 1 through 15. |
| `0x00113b84` | `AudioManager_PlaySoundData` | Starts an existing `AudioData` object on the first free effect voice. |
| `0x00113c68` | `AudioManager_StopMusic` | Stops voice 0 immediately or starts a fade-out. |
| `0x00113cc4` | `AudioManager_StopAllSounds` | Stops all 16 voices, clears instances/scheduling, and clears the music pointer. |
| `0x00113d3c` | `AudioManager_SetGlobalVolume` | Values below 2 set music/effect levels to zero; higher values reset both to 255. |
| `0x00113ddc` | `AudioManager_SetMusicVolume` | Clamps to `0..255` and updates voice 0 when master volume is greater than 1. |
| `0x00113e5c` | `AudioManager_SetEffectsVolume` | Clamps to `0..255` and updates voices 1 through 15. |
| `0x00113f00` | `AudioManager_SetVoiceVolume` | Updates one voice when master volume is greater than 1. |
| `0x00113f78` | `AudioManager_StartMusicFade` | Initializes fixed-point fade state and direction. |

Recovered helpers are `AudioManager_PlayMusicByIndex` `0x00113780`, `AudioManager_PlaySoundByIndex` `0x001137d4`, `AudioManager_IsVoiceFinished` `0x00114108`, `AudioManager_AddSoundInstance` `0x00114138`, `AudioManager_CreateSoundInstance` `0x001141b0`, `AudioManager_LoadSoundInstanceByIndex` `0x001142a4`, `AudioManager_SetListenerPosition` `0x0011437c`, `AudioManager_RemoveSoundInstance` `0x001143e8`, `AudioManager_ClearSoundInstances` `0x001144fc`, and `AudioManager_StopVoice` `0x00031ccc`. The list append helpers return the new zero-based index. Removal reindexes every following instance.

All named sound loads use directory `.\audio\`, resource type 0, and cache level 3. Music uses the music enable/level field; effects and positional instances use the effects field. A full 16-voice device therefore leaves no free effect voice, but voice 0 remains reserved for music.

### Music fades and end heuristic

Fade durations are counts of **engine audio updates**, not milliseconds. `fadeCurrent` is a 16.16 value. Fade-in starts at zero and uses:

```text
fadeStep = FixedDivide(0x00ff0000, fadeFrames << 16)
```

Fade-out starts at `0x00ff0000` and deliberately uses `0xff010000` (65281 in 16.16) as its numerator. Each update adds the positive step, sends `fadeCurrent >> 16` to voice 0, and decrements `fadeRemaining` when nonzero. The voice-volume conversion described below quantizes these levels.

When a fade-in has exhausted its countdown on a non-looping song, the manager computes:

```text
fadeStart = musicSampleCount - fadeDuration * 16000
```

Once voice position reaches that threshold, it starts a fade-out of the original duration and clears `musicData`. If `fadeStart <= 0`, the update returns without advancing the fade. A requested fade-out does **not** itself stop voice 0 when its countdown reaches zero; this original behavior is retained in the reconstruction.

## `AudioPlayer` forwarding layer

The manager owns a four-byte wrapper corresponding to S2D's `AudioPlayer`. Its only field is an `s3dAudioDevice *`. `AudioPlayer_Ctor` is `0x000a524c`, `AudioPlayer_Dtor` `0x000a52f0`, and `AudioPlayer_Init` `0x000a5364`; initialization allocates the `0x1ec`-byte device and starts its mixer thread.

| Address | Function | Forwarded operation |
|---:|---|---|
| `0x000a540c` | `AudioPlayer_Play` | Start PCM voice |
| `0x000a545c` | `AudioPlayer_Stop` | Stop voice |
| `0x000a548c` | `StopAllAudioVoices` | Stop all 16 voices |
| `0x000a54dc` | `AudioPlayer_GetState` | Finished/free flag |
| `0x000a550c` | `AudioPlayer_SetVoiceLooping` | Set loop flag |
| `0x000a5544` | `AudioPlayer_GetVoiceLooping` | Get loop flag |
| `0x000a5574` | `AudioPlayer_SetVoiceVolume` | Set voice volume |
| `0x000a55ac` | `AudioPlayer_GetVoiceVolume` | Get voice volume |
| `0x000a55dc` | `AudioPlayer_SetMasterVolume` | Set master level |
| `0x000a560c` | `AudioPlayer_SetVoiceVolumeMode` | Select direct/shift representation |
| `0x000a5644` | `AudioPlayer_GetMasterVolume` | Get master level |
| `0x000a566c` | `AudioPlayer_Pause` | Device pause |
| `0x000a5694` | `AudioPlayer_Resume` | Device resume |
| `0x000a56bc` | `AudioPlayer_Update` | Device error recovery |
| `0x000a56e4` | `AudioPlayer_GetVoicePosition` | Current sample position |

The direct-volume-mode setter and voice-volume getter have no callers in this game build. The other operations establish the actual manager-to-device boundary.

## Voice state and exact software mixing

The low-level voice-state block is embedded in `s3dAudioDevice` beginning at device offset `+0x04`. `CMixerThread` keeps a pointer to this state at its own `+0x40`. All arrays below have 16 entries and offsets are relative to the state block.

| State offset | Type | Meaning |
|---:|---|---|
| `+0x00` | `int32_t` | Master left-shift amount, initially 7; the manager changes it to 8 with level 255. |
| `+0x04` | `uint8_t` | Command-pending/wakeup flag. |
| `+0x08` | `int32_t` | Device command: 0 resume/open, 1 pause/close, 2 shutdown. |
| `+0x0c` | `CMixerThread *` | Mixer-thread object. |
| `+0x10` | `int16_t *[16]` | PCM source pointers. |
| `+0x50` | `uint32_t[16]` | PCM sample counts. |
| `+0x90` | `uint32_t[16]` | Current sample positions. |
| `+0xd0` | `int32_t[16]` | Direct volume or left-shift amount, depending on volume mode. |
| `+0x110` | `uint32_t[16]` | Loop flags. |
| `+0x150` | `uint32_t[16]` | Finished/free flags; 1 means available. |
| `+0x190` | `uint32_t[16]` | Volume modes; exactly 1 selects direct multiplication. |
| `+0x1d0` | `RSemaphore` | Thread-completion synchronization. |
| `+0x1d4` | `RMutex` | Shared voice-state lock. |
| `+0x1d8` | `int32_t` | Last stream error/status. |
| `+0x1dc` | `int32_t` | Error-recovery countdown. |
| `+0x1e0` | `RThread` | Mixer thread handle. |
| `+0x1e4` | `uint8_t` | Active/resumed flag. |

Recovered operations:

| Address | Function | Behavior |
|---:|---|---|
| `0x000a5c04` | `StartAudioVoice` | Assigns source/end, clears position and finished flag, sets loop and initial volume. |
| `0x000a5da0` | `StopAudioVoice` | Clears source/end/position, marks finished, and resets to shift-volume mode. |
| `0x000a5e78` | `IsAudioVoiceFinished` | Returns the finished/free flag. |
| `0x000a5eb8` | `SetAudioVoiceLooping` | Writes the loop flag under the state mutex. |
| `0x000a5f14` | `GetAudioVoiceLooping` | Reads the loop flag. |
| `0x000a5f54` | `SetAudioVoiceVolume` | Updates direct volume or shift according to volume mode. |
| `0x000a604c` | `GetAudioVoiceVolume` | Returns direct volume, or shift multiplied by 32. |
| `0x000a6174` | `SetAudioVoiceVolumeMode` | Selects direct versus shift volume representation. |
| `0x000a5154` | `GetAudioVoicePosition` | Returns the current sample position. |
| `0x000a60dc` | `SetAudioMasterVolume` | Converts a level to the master left shift. |
| `0x000a61d0` | `GetAudioMasterVolume` | Returns `1 << masterShift`. |

Shift conversion is not an arbitrary right-shift attenuation. Both voice and master setters increment the input and find its highest set bit, starting at bit 8:

```text
shift = floor(log2(volume + 1))
```

For the normal `0..255` range this yields 0 through 8. The mixer skips voices whose stored volume is zero. Direct mode clamps to `0..255`; shift mode clamps to `0..8`.

`MixAudioVoices` (`0x000a4220`) performs, under the state mutex:

1. Clear 400 signed 32-bit accumulators (`0x640` bytes).
2. For each active voice, accumulate mono signed 16-bit samples using either `sample * directVolume` or `sample << voiceShift`.
3. After reading the sample at the end of a source, either wrap to position zero or clear the source/count, mark finished, reset position and volume mode, and stop mixing that voice for the rest of the block.
4. Release the state mutex and convert each accumulator using the original arithmetic-shift sequence:

   ```text
   converted = ((accumulator >> 8) << min(masterShift, 8)) >> 8
   output = Clamp(converted, -32768, 32767)
   ```

5. Submit the resulting 800-byte descriptor through the stream object at mixer-thread offset `+0x10`.

At master level 255 (`masterShift = 8`), this is equivalent to an arithmetic `accumulator >> 8`; retaining both shifts matters for lower master levels and negative samples. Accumulation is 32-bit wrapping arithmetic. There is no stereo panning or resampling in this loop.

## Symbian thread, stream, and recovery

`s3dAudioDevice` creates an `RThread` named `Audio` with a `0x2000`-byte stack, minimum heap `0x100`, maximum heap `0x10000`, priority `0x1e`, and the voice-state block as its argument. `MixerThread_Entry` (`0x000a349c`) constructs the `0x5c`-byte `CMixerThread`, installs an active scheduler, and runs it until the shutdown command.

`MixerThread_ConstructL` (`0x000a3758`) configures `TMdaAudioDataSettings` at mixer offset `+0x14`:

- `iSampleRate = 0x100`, `ESampleRate16000Hz` in the S60 SDK;
- `iChannels = 0x02000000`, `EChannelsMono`;
- 400 signed 32-bit accumulation elements and 400 signed 16-bit output elements.

One submission is therefore 400 / 16000 = **25 ms**. `MixerThread_OpenStream` (`0x000a4090`) closes any old stream, sleeps 1000 units in the original code, creates `CMdaAudioOutputStream`, and opens it with these settings. Open/play callbacks call the mixer and submit the next buffer. `HandleStreamBufferCopied` additionally reconciles each finished flag with whether its source pointer is null.

`CMixerActive` is a custom active object used to deliver device commands into the mixer thread. `AudioDevice_SetCommand` (`0x000a50d0`) stores the command and wakes the thread. Command 0 opens/resumes, command 1 closes/pauses, and command 2 stops the active scheduler. `AudioDevice_Pause` and `AudioDevice_Resume` only issue a command when the active flag changes; they do not stop or reset individual voices.

Stream errors set the status and start a countdown of 90 engine updates. `AudioDevice_UpdateErrorRecovery` (`0x000a5194`) pauses/closes the stream when the countdown reaches 45 and resumes/reopens it at zero. `AudioManager_Update` invokes this recovery once per engine frame.

## Positional sound and scheduling

The `0x38`-byte sound-instance object constructed by `SoundInstance_Ctor` (`0x00116d04`) has these fields:

| Offset | Type | Meaning |
|---:|---|---|
| `+0x00` | `AudioManager *` | Owning manager. |
| `+0x04` | 3 × fixed `int32_t` | Source position. |
| `+0x10` | `int32_t` | Calculated volume. |
| `+0x14` | `int32_t` | Assigned voice, or `-1`. |
| `+0x18` | 3 × fixed `int32_t` | Listener position. |
| `+0x24` | fixed `int32_t` | Cached distance, also used for priority. |
| `+0x28` | `uint32_t` | Loop flag. |
| `+0x2c` | `AudioData *` | Sound resource. |
| `+0x30` | `uint32_t` | Manager-ownership/deletion flag; not an audible-active flag. |
| `+0x34` | `int32_t` | Index in the manager's instance list. |

`SoundInstance_Initialize` sets manager/resource/loop/ownership and resets distance and volume. Manager-created instances use ownership 1; scene-owned instances use 0 and are removed from the list without being deleted. `SetSoundInstancePosition` (`0x001170b0`) and `SetSoundInstanceListenerPosition` (`0x001171b8`) copy three fixed-point coordinates.

`CalculateFixedPointDistance` (`0x00129824`) computes Euclidean distance with 16.16 arithmetic. It uses the sum of absolute components to select an even right-shift normalization when that sum exceeds `0xb50000`, multiplies the normalized squares, uses the engine's rounded fixed-point square root, and left-shifts the result back.

`CalculateSpatialSoundVolume` (`0x00116fec`) performs:

```text
distance = Distance(listenerPosition, sourcePosition)
volume = 285 - (FixedMultiply(distance, 0x38000) >> 16)
volume = Clamp(volume, 0, 255)
```

`0x38000` is 3.5 in 16.16 fixed point. Therefore attenuation is `285 - 3.5 * distance` in the engine's position units; it is **not** division by 3.5 and has no stereo component. `UpdateSoundInstanceVolume` (`0x00116e74`) sends the result to an assigned voice.

Each manager update clears the 16-slot schedule and reinserts instances. `AudioManager_ScheduleSoundInstance` (`0x0011338c`) uses the first empty slot for nonzero-volume instances. A closer instance replaces a more distant occupant and recursively reschedules the displaced instance if it was active. Instances that cannot be scheduled are stopped but remain in the manager list. `ActivateSoundInstance` (`0x00116ec4`) acquires an effect voice when necessary, while `StopSoundInstance` (`0x00116f94`) releases the voice and sets both voice and list index to `-1`.

## Portable reconstruction and SDL3 boundary

The reconstruction now implements the recovered pipeline in:

- `src/soft3d/AudioData.*`: SAU/WAR decoding and PCM ownership;
- `src/soft3d/AudioDevice.*`: voice state, exact 400-sample mixer, mutex protection, worker lifecycle, and error recovery;
- `src/soft3d/AudioManager.*`: music/effect controls, fades, resource loading, voice allocation, and positional scheduling;
- `src/soft3d/SoundInstance.*`: fixed-point distance, attenuation, activation, and ownership behavior;
- `src/platform/dummy/DummyAudioBackend.*`: recording backend for deterministic and threaded tests;
- `src/platform/sdl/SdlAudioBackend.*`: SDL stream/device lifecycle and PCM submission.

`AudioBackend` is the platform boundary corresponding to the device stream, not to SDL's voice mixer. `s3dAudioDevice` retains a worker analogous to `CMixerThread`; SDL receives completed **mono signed-16-bit 16 kHz** PCM through `SDL_PutAudioStreamData`. SDL's stream handles device-format conversion only. Pause/resume closes and recreates the SDL stream, matching the original command semantics rather than resetting voices. A null backend leaves the manager and manually driven `MixBlock` usable for tests and silent builds.

`tests/test_audio.cpp` checks exact direct and shift arithmetic, negative rounding, saturation, looping and completion, mode reset, voice allocation, real `7DaysPiano03.sau` playback, fade quantization, global-volume gating, fixed-point distances, positional muting, threaded pause/resume, and backend start failure. CTest also runs the SDL app with dummy video and audio drivers so the actual SDL stream lifecycle is exercised headlessly.
