# Symbian3rd license, verification, device services, and vibration

## Scope and evidence

Stage 14 covers the game-owned license state machine and its platform consumers in the ARMv5 Symbian3rd build. The behavioral authority is the current `out.bin`, which is a **cracked build**. Observed crack behavior is preserved separately from inferred pre-crack behavior; no unmodified binary is available to prove the original instruction at the crack site.

Portable protocol/game code is in `Reversed_Game/src/game/License.*`. Platform interfaces are in `src/platform/PlatformServices.hpp`, deterministic backends in `src/platform/dummy/DummyServices.hpp`, Linux host services in `src/platform/linux/LinuxServices.hpp`, and SDL haptics in `src/platform/sdl/SdlVibrationBackend.*`. The recovered DES block implementation and its 512-word SP table are in `License.cpp` and `DesSpTable.inc`; the code carries Eric Young's libdes attribution.

## Suspected license-check crack

The helper at native `0x0000d52c` (Ghidra `0x0001552c`) scans a 16-word verification array. Its second argument is stored but never read:

```c
int index = 0;
while (index < 16 && words[index] == 1) {
    ++index;
}
*result = index == 9 ? 1 : -index;
```

The decisive instruction is native `0x0000d590` (Ghidra `0x00015590`): `cmp r3,#9`. Every scene and door caller supplies a slot argument, and the surrounding loop makes `index == requestedSlot` the plausible parameterized form. The hard-coded 9 is therefore treated as a suspected crack modification, **not** as proven original behavior. There is no evidence that `index == requestedSlot` was the instruction in the retail binary; that form is an inference from the call pattern.

The reconstruction preserves the observed helper as the default `LicenseCheckPolicy::CrackedBinary`. `Parameterized` exists for comparison tests, and `OfflineAuthorized` is an explicit host/test-only policy. Neither alternative is the default.

The same helper also makes every `result == 0` branch in `MyClientSession_HandleResponse` unreachable: its only possible results are 1 and nonpositive scan indices. Those branches are preserved as native control flow and are not described as successful activation paths.

## Verification state and gates

`License_SetVerificationWord` (`0x0000d5c0`) writes 1 to the selected array word. Response records select that index from the server. The door at unit ID 10000 checks slot 7 and invokes `Scene_BeginAuthorization` when the result is below 1. This door is in day 1, so the nag can block day-1 progress even though ordinary day-1 scene phases bypass the day gate.

Scene phases use distinct slots:

| Consumer | Slot | Native behavior when unauthorized |
|---|---:|---|
| `Scene_Update` | 7 | Begins authorization and skips world update. |
| `Scene_PostUpdate` | 9 | Begins authorization and skips post-update. |
| `Scene_PrepareRender` | 12 | Begins authorization and skips world render preparation. |
| `Scene_SubmitRenderables` | 14 | Begins authorization and draws the authorization screen instead. |
| `DoorTrigger_Update`, ID 10000 | 7 | Begins authorization and clears interaction input. |

Days after day 1 enter the gate when the check fails and authorization is not active. Day-1 scene phases otherwise continue normally. Scene `0x6b04` is the authorization-active flag and `0x6b08` is the registration state; the earlier portable `authorizationTimer` name was incorrect and has been removed.

`OfflineAuthorized` is deliberately different from the recovered protocol: it returns 1 for every query. It is used by existing later-day combat/map tests so they can test their intended subsystem without turning the application default into an unconditional-success shortcut.

## Client and transport classes

The portable boundary preserves protocol behavior without reproducing Symbian active objects:

- `ClientSession` owns the selected `SessionDevice` and request/response streams.
- `MyClientSession` adds device identity, initial/session DES schedules, the verification bitmap, and game result consumers.
- `SessionDevice`, `HttpSession`, `TCPSession`, `CSocketReader`, and `CSocketWriter` implement connection, socket reader/writer, cancellation, and asynchronous completion in the original. These remain platform details; portable code uses `NetworkSession`.
- `CIMEI` uses `CTelephony::GetPhoneId` into two 256-byte identity buffers. Portable code uses `DeviceIdentityProvider`.
- `CSmsSessionObserver` uses `RSendAs`/`RSendAsMessage` and reports 0 pending, 1 completed, 2 failed. Portable code uses `SmsSession` with the same status values.
- `CPhoneObserver` watches telephony state for lifecycle handling; it is not a vibration backend and does not contain license semantics that belong in the portable game layer.

The game constructs TCP transport in this build. HTTP and TCP share the generic session interface, but the portable reconstruction does not claim a live implementation of either. A host can inject a recorded or real `NetworkSession`; the default is an explicit unsupported network result.

## Cryptography and packet records

The protocol uses Eric Young's libdes DES implementation with a raw 128-byte schedule (32 little-endian 32-bit words) and MD5 digests. `DesSpTable.inc` is the native 512-word SP table from file offset `0x1324f0`. The implementation preserves libdes's initial/final half ordering and reverse decryption schedule. It is not replaced with an incompatible OpenSSL schedule conversion.

The initial schedule is zero-filled except its first 16 bytes:

```text
64 69 6e 67 6f 6f 67 61 6d 65 0f 37 64 61 79 73
 d  i  n  g  o  o  g  a  m  e     7  d  a  y  s
```

Response code 100 carries a new 128-byte session schedule encrypted with the initial schedule. Subsequent records use the session schedule. `message.sdt` contains exactly two 256-byte encrypted message buffers. `GameEngine_Init` reads them from `common/message.sdt`, with an external-file fallback and failure result 3 when neither source is available. `Scene_SendAuthorizationSms` decrypts both buffers and passes them as recipient/body to the SMS service. Their opaque bytes are not treated as SDT text tables.

`domain.sdt` is a 36-byte shipped record. No `domain.sdt` string or consumer exists in this ARM build. It is therefore retained as a real fixture but not assigned an invented runtime role.

### Requests

All integer fields in protocol records are big-endian; DES block data and schedules use the native libdes byte layout.

- Service request: `uint16 0`, 64 zero-padded bytes containing `com.dingoo.server.VerifyService`, one zero byte, then the same 64-byte service field. Plaintext is 131 bytes. The body after the two-byte type is zero-padded to 136 bytes and DES-encrypted with the initial schedule. The outer packet is `uint16 length=138`, `uint16 type=0`, and the 136-byte ciphertext, totaling 140 bytes.
- Code-100 follow-up: `uint16 1`, `uint16 1`, and the 16-byte MD5 of the 256-byte device-identity buffer. Its framed packet is 28 bytes with outer length 26.
- Verification request: `uint16 2`, `uint16 operation`, MD5(text), MD5(identity buffer), encrypted with the session schedule.

For every request, the first two plaintext bytes remain unencrypted in the packet. Only the following bytes are DES-encrypted, padded with zeros to an eight-byte boundary. The outer length includes those two bytes plus the encrypted body.

### Responses

The response's first two bytes are an unencrypted big-endian code; the remaining bytes are DES-encrypted with the initial schedule before code 100 and the session schedule afterward.

- Code 100: at least 128 plaintext bytes, copied as the new session schedule, followed by the code-100 follow-up request.
- Code 101: `int32 operation`, `int32 verificationIndex`, then optional opaque payload. The native payload consumer treats it as an STX texture and persists a registration record. Portable code exposes the payload to a handler instead of pretending that parsing succeeded.

Operation 1 updates the initial menu result when `menuRegistered` is false; otherwise it updates scene registration. Operation 2 updates scene registration. Native result states are 2 and 4, with state 6 on the special operation-2 path. The random anti-cheat result branches that compare against zero are unreachable with the recovered helper, as noted above.

`GameEngine_LoadRegistrationData` (`0x0006fe50`) and `GameEngine_SaveRegistrationData` (`0x0006fa28`) use a 248-byte record: a 128-byte schedule, two 16-byte MD5 values, a 24-byte encrypted STX payload, and 64 bytes of registration state. The encrypted filename is decrypted with the initial schedule. The portable stage implements protocol/state and payload handling but does not claim compatibility with an unverified plaintext filename or registration file generated by the cracked binary.

## Main-menu authorization

`MainUIDlg` mode 4 calls `MainUIDlg_DrawAuthorization` (`0x000507f4`). It loads `ui/shouquan.stx`, draws it centered horizontally and five pixels above the bottom, and places secondary-font text inside its margins. Substates and text IDs are:

| State | Text ID | Behavior |
|---:|---:|---|
| 0 | 400 | Back resets the session and selects state 2. |
| 1 | 401 | Accept starts a connection; back selects state 2. |
| 2 | 402 | Accept leaves for the main menu. |
| 3 | 404 | Accept retries; back selects state 2. |
| 4 | 405 | Accept leaves for the main menu. |

After the sound prompt, native `MainUIDlg_Update` advances from mode 3 directly to main-menu mode 5; it does not enter authorization mode 4. Mode 4 remains reachable only through the explicit authorization path. Backing out of state 0 reaches state 2 and then the main menu without inventing a license-success result. Unsupported connection attempts select state 4. `MainUIDlg::ShowAuthorization` provides an explicit host/test entry point for the recovered screen. Authorization text uses `0xffff` white, matching `PackRgb565(255,255,255)` in the native renderer.

## Scene authorization

`Scene_BeginAuthorization` sets active=1 and registration state=1. State 1 draws string 410 and soft-key labels 1/2. Accept enters state 2 and starts SMS authorization; back returns to the menu.

State 2 draws strings 411, 413, and 415 plus retry/elapsed numbers, and has a 2880-frame timeout. Accepting state 1 does not also run a state-2 retry in that frame. On each completed SMS status, the scene checks slot 0. A negative result increments the attempt count and sets the word at `-result`; if the next check returns 1, state 4 is selected and active is cleared. Otherwise another SMS is sent. SMS failure or timeout selects state 5, which draws string 414 and returns to the menu on back. A server-driven state 4 does not by itself clear the active flag; local state-2 success does.

The portable Linux SMS stub completes sends deterministically without claiming to use a modem. With the default cracked helper, successful completion takes nine state-2 updates and reaches attempts 10 while filling words 0–8. With the parameterized comparison helper, an initially empty bitmap completes on the first update.

## Device identity and vibration policy

The default Linux identity provider returns the explicit test IMEI `001001000000007`, which passes the Luhn test. It is not read from the host or presented as a real phone identity. The default network backend remains unsupported. Tests inject deterministic identity, network, and SMS backends for success, malformed response, failure, pending, cancellation, and missing-capability cases.

Native `GameEngine_SetVibrationEnabled` and `GameEngine_IsVibrationEnabled` only persist/read the configuration word at manager `0x2d4`; no native vibration call or HWRM import was found in this binary. The reconstruction therefore does not invent original vibration events. It provides an explicit `Vibrate(duration)` host API, disabled by the same configuration flag, with an unsupported result when no device exists.

SDL3 3.2.10 haptics are available in the build environment. `SdlVibrationBackend` initializes `SDL_INIT_HAPTIC`, opens the first device supporting `SDL_InitHapticRumble`, and uses `SDL_PlayHapticRumble`. It reports unsupported when no such device exists and stops rumble when disabled. Deterministic dummy pulses are recorded only when supported and enabled.

## Verification

`test_game_license.cpp` covers:

- independent libdes decryption/encryption fixtures, zero-key behavior, and round trips;
- real `message.sdt` and `domain.sdt` bytes;
- MD5 vectors, service/request framing, code-100/101 handling, malformed responses, and session-key installation;
- cracked, parameterized, and offline-authorized policy differences;
- SMS success, retry, failure, timeout, pending/cancel, and missing identity/network/SMS support;
- the Luhn-valid Linux test identity and vibration support results;
- real engine-frame day-1 nag, later-day gating, offline contrast, SMS completion/failure, menu connection failure, and exact framebuffer hashes.

Exact hashes are scene prompt/progress `0xd893fa55` (both screens currently produce the same dummy-presented output), scene failure `0xbc523725`, and menu failure `0x4f70a5fd`. The direct menu test clears the framebuffer before entering mode 4 because that mode preserves the previous startup background.

The default suite has 38 tests; SDL-independent variant suites have 31. Tests run serially because existing fixed temporary filenames can collide.
