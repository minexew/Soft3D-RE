# 7 Days: Salvation — correctness, refactoring, and polish roadmap

## Purpose

This roadmap follows the completed stage-16 reconstruction. Its purpose is to turn the broad automated coverage into demonstrated gameplay correctness, complete the useful reusable Soft3D surface and remaining game-object APIs, make the source maintainable, and only then add developer and remake-oriented features.

The current scheduling priority is to finish the native SDK and game-object inventories and recover their confirmed missing APIs before extensive revalidation. The reported first-room progression failure remains the first correctness scenario when behavioral validation resumes. A full human playthrough is not a substitute for auditing the portable implementation against ARM, and refactoring before that audit would make regressions difficult to attribute.

The faithful configuration remains the compatibility baseline:

- ARM disassembly/decompilation is the behavioral authority; shipped resources and walkthroughs provide scenarios and coverage evidence.
- Preserve native update order, fixed-point overflow/truncation, timing, resource lifetime, save bytes, and observable quirks unless a difference is explicitly documented as a portable safety fix.
- Default builds preserve 320×240 rendering, original lookup tables, release-like disabled logging, and the disabled native debug console.
- New behavior, cheats, assists, filters, and presentation changes must be opt-in and preferably live in the application or platform layer.
- Do not recover Symbian application/UI boilerplate, C++ runtime, vendor code, or platform services as reusable Soft3D APIs. Do not invent implementations for original empty methods or duplicate compiled bodies.

## Session contract

Each numbered item below is intended to fit one focused LLM session. A session should normally touch one subsystem or a small set of related files, not an entire phase.

Every implementation session must leave behind:

1. A scoped evidence ledger mapping native addresses to portable functions, with one disposition: **matches**, **corrected**, **original no-op**, **intentional portable safety difference**, or **unresolved**. No unresolved item may be hidden by a passing test.
2. Directed tests whose expected values are derived from ARM or native data, not copied from current portable output. Existing exact hashes may only be changed after explaining the native evidence.
3. Real engine-frame coverage for behavioral changes, plus focused tests for branches that real content does not reach.
4. Corrected Ghidra names, prototypes, types, variables, comments, and tags where the investigation changes native understanding; save and read back the program.
5. Updates to the relevant existing `Reverse_Docs` document. Historical stage-completion reports should receive a correction note rather than being rewritten as if the correction had always been known.
6. A short handoff in the relevant ledger: changed behavior, new regression tests, remaining hypotheses, and the next blocking scenario.

Run the default CTest suite and the affected focused/sanitizer suites in every implementation session. Run the complete serial matrix—default, core, STDIO logging, host trigonometry, WAR, ASan/UBSan, applicable English-layout tests, and both Python suites—at every phase gate and after persistence, arithmetic, renderer, or build-option changes. Current counts are a baseline, not a target to preserve by deleting tests.

If a session uncovers too many divergences, stop at a coherent subsystem boundary and add a follow-up session. Do not finish early with a superficial audit or mix broad cosmetic edits into a correctness fix.

## Bug severity and completion gates

- **P0 — blocking:** crash, hang, softlock, impossible progression, unloadable required map/script, broken required interaction, or save corruption.
- **P1 — major:** wrong combat/QTE outcome, broken day/ending transition, missing required item, incorrect unlock persistence, or a workaround is required to continue.
- **P2 — nonblocking correctness:** wrong visual/audio presentation, minor state error, incorrect optional behavior, or misleading UI that does not prevent completion.
- **P3 — polish:** maintainability, diagnostics, optional quality-of-life work, and enhancements.

A phase is complete only when its gate is met:

- **Automated parity gate:** every reachable portable subsystem has a complete ledger, no unresolved P0/P1 divergence remains, and the full test matrix passes.
- **Completable gate:** a continuous human new-game-to-ending run succeeds without debug jumps or cheats, with save/load continuation checked on every day; zero open P0/P1 issues.
- **Content gate:** every walkthrough-listed extra event, unlock, alternate outcome, and persistent extra has a recorded successful result or a fixed/retested issue.
- **Refactor gate:** no behavior or serialized-byte change without separate native evidence; full matrix and representative human smoke scenarios pass.
- **Release-candidate gate:** all known P0/P1/P2 issues are fixed or explicitly accepted with evidence, documentation/Ghidra agree with source, and a final human regression run passes.

---

## Phase A — Reproduce, observe, and unblock the first room

### Session 01 — Freeze the baseline and build the validation ledgers

Create `Reverse_Docs/portable_parity_ledger.md` and `Reverse_Docs/human_validation.md`. Record the current source revision, build options, test results, first-room reproduction steps, and exact failure point. Build a walkthrough-derived content checklist with day, episode, map, prerequisites, expected items/flags/events/unlocks, and save-persistence expectations.

Define a common bug packet: severity, build/resource variant, save before the failure, input replay if available, frame number, episode/script command, map/unit IDs, screenshot/video, log, and native addresses implicated. Link the stage-16 audit and explicitly carry forward its limits: fixed frame budgets are not dynamic branch coverage or human completion evidence.

**Exit:** one deterministic first-room failure packet, a parity-ledger template covering all source subsystems, and a content matrix covering the whole walkthrough.

### Session 02 — Add nonintrusive replay and coverage diagnostics

Add an application-level input/event recording and replay facility with a build/version/resource identifier, initial save or new-game state, frame-indexed engine keys, and deterministic termination. Add opt-in tracing for frame, episode, map, script task/command ID, scene transition, player state, and unit creation/destruction. Add dynamic script-command coverage alongside the existing static host-call inventory.

Keep diagnostics out of the default frame path unless explicitly enabled. Use unique per-run output paths. Add replay determinism tests and prove that recording disabled produces the same state/frame results as the uninstrumented baseline. Do not add an ARM emulator dependency merely for tracing; use static ARM comparison and portable traces first.

**Exit:** the first-room failure can be replayed from a small artifact, and the trace identifies the last successful and first divergent script/scene/player operations.

### Session 03 — Audit and fix the complete first-room critical path

Trace new-game startup through `ep0`, initial SBP loading, spawn, movement, collision, interaction, required triggers, and the exit transition. Compare every implicated portable branch and argument against ARM. Fix the root causes rather than special-casing the first room, and add one real end-to-end regression that starts a new game, performs the required interactions, and enters the next required state/map.

Check nearby paths likely to share the defect: input masks, trigger bounding tests, player state transitions, script command argument order, map/door finalization, and fixed-point coordinate conversions. Do not close this session merely because a debug map jump works.

**Exit:** a human can leave the first room through normal controls; the real engine-loop regression passes; all implicated native/portable functions have ledger entries.

---

## Phase B — Automated ARM revalidation of reachable portable logic

These sessions audit complete scoped implementations, not only functions exercised by current tests. Human smoke testing may run in parallel, but a full human validation claim waits for this phase's gate.

### Session 04 — Fixed-point scalar arithmetic and trigonometry

Audit all reachable operations in `Fixed.*`, including signed wrapping, shifts, division by zero, square root/inverse square root, sine/cosine/tangent tables, arc tangent/cosine, angle masks, and polynomial coefficients. Add directed boundary, overflow, negative, zero, and table-index fixtures calculated from ARM.

Separate reachable-parity work from recovery of additional unused SDK math in Phase D.

**Exit:** every current scalar fixed-point helper has native evidence and exhaustive practical boundary coverage; host-math remains an explicitly nonfaithful variant.

### Session 05 — Base IO, streams, archives, resources, and images

Audit `s3dBase`, narrow/wide IO, file and memory streams, SPK layouts, resource ownership/cache levels, SAU/WAR data ownership, STX/TGA loading, and error/sentinel behavior. Recheck malformed-input guards to ensure they do not change valid native behavior.

Include cache eviction, repeated map/day teardown, file truncation, endian details, path normalization, and English/Chinese archive differences.

**Exit:** all functions in this resource/IO scope are classified, exact stream/archive fixtures pass, and sanitizer runs show no lifetime or bounds errors.

### Session 06 — Engine lifecycle, input, timing, queues, and resource presentation

Audit engine construction/shutdown, new-game/menu transitions, 23-key state transitions/repeats, 17 FPS timing, update ordering, render queue capacities/order, viewport/orientation propagation, and dummy/SDL backend contracts. Distinguish original behavior from application conveniences.

Exercise suspend/resume/quit, return to menu, repeated new games, simultaneous keys, handedness changes, and queue boundaries through real frames.

**Exit:** lifecycle/input/queue behavior has complete ledger coverage and deterministic frame tests, with no concrete game rules introduced into `soft3d`.

### Session 07 — SST VM, task lifecycle, and GameSI marshalling

Audit `SimpleScript.*` and the generic portions of `GameSI.*`: typed values/references, conditions, nested bodies, calls, stack/local/global state, task scheduling, suspension, timers, `runTask`, shared words, reset, and sidecar interactions. For all 218 callback slots, verify native address, C++ signature, argument conversions, return value, context pointer, and error behavior; command-specific behavior is audited in its owning subsystem session.

Use the dynamic coverage from Session 02 to identify conditionals and tasks never reached by shipped fixed-frame runs. Add synthetic native-derived cases for those branches.

**Exit:** generic VM and all dispatch signatures are accounted for; static and dynamic coverage are reported separately; no silent unknown-command path remains.

### Session 08 — Scene, day/map loading, camera, doors, and presentation sequencing

Audit `Scene.*`, `CameraController.*`, menu-backdrop integration, day/episode selection, SBP finalization/fallback, spawn/direction, eight camera modes, SPL completion/projection, door transitions, fades/titles/game-over, environment/listener scheduling, and scene shader selection.

Test every transition with subsequent frames and repeated teardown, not just arrival at a target map. Pay particular attention to stale animation/resource references and camera/listener restoration.

**Exit:** all scene/camera functions are classified; every day/map/door mode has directed coverage and real transition frames.

### Session 09 — Generic units, physics, BSP collision, and paths

Audit `Units.*`, `StateMachine.hpp`, `BspCollision.cpp`, and relevant BSP queries. Verify message ordering, manager buckets, state-template enter/update/exit order, transforms, LOD, simple/slide movement, ground/shadow traces, leaf/brush tests, and path edge weights/sentinels.

Use synthetic boundary geometry plus real maps. Include zero-length movement, exact plane contact, path endpoints, disabled collision, and pooled-unit reuse.

**Exit:** all generic movement/collision/path behavior is native-checked and covered by state-exact tests through real unit updates.

### Session 10 — Map units and all interaction triggers

Audit `MapUnits.*` and `Triggers.*`: every Things variant, BspUnit, ElectricSaw, Trap, StayBox, all nine trigger classes, prompts/choices, item requirements, door state, hide/item/talk behavior, camera/light restoration, update pre-passes, enable/removal, and second-save fields.

Verify real SBP entity records as well as synthetic entry/exit and cancellation boundaries. Preserve confirmed original double updates and no-op methods.

**Exit:** every map-unit/trigger virtual and command consumer is classified; real maps and all interaction state machines pass end-to-end tests.

### Session 11 — Player, sword, and all player states

Audit `Player.*`, `PlayerStates.*`, and `Sword.*`: all 20 states and virtual slots, input/control masks, animation windows, melee/gun hit tests, hurt/death variants, QTE, hiding, FPS mode, pushing, inventory snapshots, and script commands.

Test transition chains and interruption at native frame boundaries, including simultaneous damage/QTE/door/trigger interactions. Use real assets and engine input for representative encounters.

**Exit:** every player/state/sword function has a ledger entry and transition-exact tests; no first-room workaround remains in player logic.

### Session 12 — Common enemy AI, pooling, perception, and combat

Audit `Enemy.*` common behavior and the corresponding manager pools: construction configuration, spawn/reset, hearing/sight, waypoints/stay boxes, attacks, damage/death, blood splats, rewards, nondeleting reuse, map spawning, and persistence extensions.

Include pool exhaustion, singleton selection, linked StayBoxes, repeated day preparation, and real map combat.

**Exit:** all common enemy/pool functions are native-classified before variant audits begin; common behavior is not used to explain away a variant override.

### Session 13 — Maid and Bully families

Audit every override in `MaidBully.*`, including resource tables, extra sounds/models, hit windows, FatMaid projectiles, Day3 charge/QTE/death, Day5 saw/charge/pickup/slam behavior, and corrected gun rewards.

Exercise each class in its real day/episode context and through subsequent save/load frames where applicable.

**Exit:** all seven classes have per-override evidence and encounter tests, with no inherited generic substitute.

### Session 14 — Worm, Ghost, Lamper, and Weed

Audit `SpecialEnemies.*`: random Worm colour/scale/speed and jump/landing/death, Ghost invisibility/immunity/Twist/proximity audio, Lamper point hits and one-hit death, and Weed perception/custom damage.

Check deterministic RNG injection without changing native random-call ordering, and exercise shipped encounters on their native maps.

**Exit:** every special-enemy override and random boundary has native-derived tests and real encounter coverage.

### Session 15 — Named enemies, Lord, and Boss

Audit `NamedEnemies.*`, `Lord.*`, and `Boss.*`: Victor phases, Sam blending, Prisoner idle/death/reward, Kate/Bully linkage, DarkKen region graph and dual guns, Lord's complete crystal/equipment/corpse sequence, and Boss action/delay/death behavior.

Cover cancellation, linked-actor removal, phase interruption, and final transitions, not just successful happy paths.

**Exit:** all seven classes and every linked encounter phase are classified and tested through real engine frames.

### Session 16 — Generic UI, menus, fonts, configuration, and extras storage

Audit `Font.*`, SDT dialogs/blitters, `MenuDialogs.*`, `GameUIDlg.*` generic behavior, `GameConfig.*`, and `GameContent.*`. Recheck text ranges/wrapping, key interpolation, clipping/scaling, sound setup, handedness, all menu paths, model/document/photo storage, and configuration/save-header separation.

Retain exact native pixels for the faithful configuration and preserve original no-op rank behavior.

**Exit:** generic UI/menu/configuration functions are fully classified; fresh and completed profiles round-trip without cross-contaminating slot state.

### Session 17 — Concrete GameUI, items, maps, statistics, and unlock progression

Audit `GameUiController.*` and game-facing item/resource tables: HUD/prompt/dialog/QTE rendering, nine-slot inventory, every item use/combination, files, map calibration/markers, about models, statistics, unlocks, and related GameSI commands.

Create one end-to-end test per walkthrough progression family: acquire → use/combine → world effect → save/reload → unlock/extra availability.

**Exit:** every item and unlock consumer has a native-backed result; all content-matrix prerequisites can be inspected and tested without relying only on screenshots.

### Session 18 — Full save/load and deterministic continuation

Audit `SaveGame.cpp`, generic archive state, every concrete unit extension, slot headers, script sidecars, reconstruction links, and second-save ordering. Verify all native writer/reader asymmetries and documented portable completion fixes separately.

For every day and representative encounter, test save → load → frames → second save, plus truncation at every section, invalid links/types, missing sidecars, and continued gameplay. Include unlock/config interactions and new-game reset after loading.

**Exit:** exact expected bytes are native-justified, every reconstruction family is covered, and no loaded state requires a debug repair to continue.

### Session 19 — Objects, models, animation, effects, and render-state ownership

Audit `Object*`, `Animation`, `Texture*`, `Renderer`, and `Effects`: model/material loading, streams/bones, transforms/bounds, animation interpolation/wrapping, texture lifetime, sorting/queues, lensflare/billboard/fake-shadow/gunfire, and resource teardown.

Use real animated SOJ/SAI/STX resources and exact frames at native animation boundaries.

**Exit:** all object/effect/render-state functions are classified, with no unexplained visual hash changes or stale cache ownership.

### Session 20 — Rasterizer, BSP rendering, and presentation shaders

Audit `Raster.*`, `BspRender.cpp`, `BspEffects.cpp`, `BasicShader.*`, and `PresentationShaders.*`: clipping, selectors, winding, depth, blending, lightmaps/styles/PVS, blood splats, orientation traversal, history buffers, and terminal states.

Add directed native-derived pixels for untested selectors/branches and arbitrary uncaptured real scenes identified during human smoke tests. Do not claim exhaustive pixel parity from hashes alone.

**Exit:** all scoped rendering functions are classified; every implemented selector/effect has a directed test and all known visual discrepancies have owners.

### Session 21 — Audio, license policy, and portable platform services

Audit audio decode/mixing/voices/fades/positional behavior and music/environment integration, then license DES/MD5/protocol/gates plus device/network/SMS/vibration backends. Keep these as two review tracks within the session and split it if either produces substantial corrections.

Verify final-word decoder boundaries, 16-voice stealing/mixing, authorization retry/cancel/timeout, default cracked-binary policy, explicit test-only offline policy, and unsupported host capabilities. Do not require live Symbian services for parity claims.

**Exit:** all remaining reachable portable functions are classified; the full validation matrix passes and the automated parity gate is reviewed for omissions.

---

## Phase C — Human completability and content validation

Human play happens outside the LLM. Each session below consumes a bounded human test packet, fixes its issues, adds regressions, and produces the next test packet. Debug jumps may isolate a scenario, but cannot satisfy the continuous-run requirement.

### Session 22 — Human days 1–2: blocking-path validation

Test normal new-game progression, required interactions/combat/QTEs, doors, saves, and day transitions. Start without cheats and record every divergence from expected walkthrough behavior.

Fix all P0/P1 findings before expanding scope. Add the smallest real-script regressions that reproduce each fixed failure.

**Exit:** days 1–2 pass sequentially from a new game, including reload continuation, with no open blocker.

### Session 23 — Human days 3–4: encounter and branch validation

Continue the same profile through days 3–4, emphasizing special encounters, QTE failure/success, hiding, items, alternate script branches, and save/load before and after major encounters.

**Exit:** days 1–4 remain completable in one continuous profile; every fixed issue has an automated regression.

### Session 24 — Human days 5–7 and endings

Complete days 5–7, boss/Lord encounters, game-over/restart checks, ending transitions, credits/titles, and return to menu. Test intended failure branches separately from the successful continuous run.

**Exit:** at least one new-game-to-ending run succeeds without debug intervention and all seven daily save continuations pass under human control.

### Session 25 — Independent continuous completion rerun

Start from a fresh configuration and repeat the entire game without relying on prior debugging state. Vary save/load points, menu returns, orientation, and at least one death/retry per relevant encounter family. This run catches order-dependent flags and state leakage that isolated day tests miss.

**Exit:** the completable gate passes on a clean profile with zero open P0/P1 issues.

### Session 26 — Walkthrough extras and unlocks: days 1–3

Execute every listed optional event, hidden interaction, document/model/photo unlock, alternate choice, and missable item for days 1–3. Check both acquisition and persistence after save, reload, day transition, and return to menu.

Update the content matrix with exact steps and evidence; if the walkthrough is ambiguous, record the interpretation rather than silently dropping the entry.

**Exit:** every days-1–3 content-matrix row is passed or has a tracked, reproducible defect.

### Session 27 — Walkthrough extras and unlocks: days 4–7 and alternate outcomes

Cover remaining optional events, good/extra ending requirements, alternate QTE/combat outcomes, extras-gallery availability, and fresh-versus-completed configuration behavior. Recheck earlier-day prerequisites from a clean profile where required.

**Exit:** the content gate passes; no walkthrough-listed feature is supported only by a synthetic command test.

### Session 28 — Nonblocking bug burn-down and final human regression

Group remaining P2 findings by rendering, audio, UI, and minor gameplay logic. Fix them with native evidence and exact directed regressions; avoid unrelated refactoring. Then rerun the continuous completion path and a focused all-content checklist affected by the fixes.

**Exit:** zero unreviewed P2 issues remain, full automated and human release-candidate gates pass, and known accepted differences are explicitly documented.

---

## Phase D — Useful Soft3D SDK completeness

“Full Soft3D” here means all useful reusable SDK facilities evidenced by the analyzed build, including uncalled methods—not runtime, Symbian shell, invented APIs, or arbitrary dead code. Existing tests and source names alone cannot identify the missing surface.

### Session 29 — Native-to-portable SDK inventory and exclusion audit

Create `Reverse_Docs/soft3d_sdk_inventory.md`. Crosswalk native functions, RTTI classes, and vtable slots against `src/soft3d`, including zero-caller functions. Classify each as implemented, missing useful API, original empty, duplicate compiled body, game-specific, platform-specific, runtime/vendor, data/false function, or intentionally omitted.

Use function bodies, RTTI, call patterns, and S2D lineage as clues; do not assume an S2D declaration exists in Soft3D without native evidence. Prioritize fixed-point operations and reusable utilities with clear semantics.

**Exit:** every candidate has an address, owner, native evidence, disposition, and planned test; no unnamed useful class/API remains unreviewed.

### Session 30 — Recover missing fixed-point scalar and vector APIs

Implement the inventory's confirmed missing scalar math, conversions, angles, Vec2/Vec3/Vec4 helpers, interpolation, and related utilities in their native ownership layer. Preserve exact wrapping, rounding, shift masks, sentinel behavior, and error results.

Add synthetic tests because these functions may have no current callers. Do not rewrite working call sites merely to advertise newly recovered APIs.

**Exit:** all confirmed useful scalar/vector candidates are implemented or explicitly excluded, with native-derived tests and saved Ghidra documentation.

### Session 31 — Recover missing matrix, geometry, camera, and path APIs

Recover confirmed unused matrix transforms/factories, projection/view helpers, bounds operations, Bezier degree/edit/bounds utilities, and camera-path/storage methods not already present. Keep native argument order and mutation semantics.

Test identity, composition order, singular/degenerate inputs, clipping/bounds, and exact fixed-point results.

**Exit:** the math/geometry/camera inventory is closed with useful APIs callable by external hosts through real tests.

**Status:** Complete. The evidence ledger is in `soft3d_sdk_inventory.md` and `soft3d_geometry.md`; recovered public APIs live in `Fixed.*`/`Geometry.*`, with directed coverage in `test_math.cpp` and `test_geometry.cpp`. Existing Bezier/SPL/camera-path storage required no additions. Native type/name corrections and broken allocating-clip exclusions are explicit. The complete default/core/STDIO/host-math/WAR/ASan-UBSan/applicable-English/Python validation matrix passes.

### Session 32 — Recover unused resource, model, texture, and renderer APIs

Audit and recover useful uncalled methods for resource/cache control, Object/material/animation manipulation, texture/image operations, render-state setup, and render submission. Avoid platform-specific scanline/runtime support and avoid merging intentionally duplicate native bodies.

Exercise each API through public headers and real or synthetic resources, including teardown and repeated calls.

**Exit:** no useful uncalled resource/model/renderer candidate remains as “unknown”; API ownership and lifetime are documented and tested.

### Session 33 — Recover remaining streams, text, UI, audio, effects, and utility APIs

Process the remaining confirmed reusable candidates by family: streams/IO/formatting, font/string/dialog accessors, audio controls, renderable effects, memory diagnostics, and small utility methods. Skip a family with no candidates rather than inventing work.

**Exit:** every useful API has a consumer-style synthetic test and public declaration; original no-ops remain empty and documented.

### Session 34 — SDK completeness gate and external-consumer validation

Re-run the inventory against source and Ghidra. Build a small external-style consumer against `soft3d` without linking `seven_days_game`, proving include hygiene, ownership, and availability of the recovered unused APIs. Update `Reversed_Game/README.md` and the format/API docs without claiming support for another engine game solely from shared lineage.

**Exit:** all native candidates are dispositioned, all “missing useful” items are resolved, the full matrix passes, and the standalone engine consumer links and runs.

### Session 34A — Game-object API completeness

Use `Reverse_Docs/game_object_inventory.md` to recover game-owned methods and non-RTTI helpers excluded from the reusable Soft3D SDK. Complete the missing ControlMap/Input query, event, and gate APIs; player/camera wrappers and trigger lifecycle; and the distinct alternate DarkKen comparator. Resolve the PlayDlg option fields and `GameUI_GetNative80` from native field consumers before implementing them; leave unresolved reserved setters out.

Preserve native ActionUp behavior for primary-only and both-bindings-up cases, and cover it through real QTE frames. Do not move game policy into `soft3d` to share code, merge behaviorally distinct comparators, or give reserved fields invented semantics.

**Exit:** every game-object inventory row is implemented or explicitly excluded with evidence, focused input/player/camera/menu/enemy tests and real engine frames pass, and the full matrix passes.

---

## Phase E — Structural cleanup and type safety

Cleanup is behavior-preserving unless a separately documented native correction is required. Use small mechanical commits/patches and exact regression fixtures; never combine a magic-number sweep with a parity fix in the same change.

### Session 35 — Engine constants, limits, flags, and sentinels

Inventory magic numbers in core engine modules. Introduce named constants or scoped enums only where semantics are native-confirmed: result codes, queue/bucket limits, texture/material flags, stream sentinels, script opcodes, input keys, and special indices.

Give enums explicit underlying types where serialized or ABI-relevant. Keep native numeric values visible in docs/tests and do not rename arbitrary literal-pool values without a consumer-backed meaning.

**Exit:** the scoped engine surface has no unexplained repeated semantic literals and all default state/frame/stream fixtures are unchanged.

### Session 36 — Scene, camera, player, and UI state enums

Replace integer modes with descriptive scoped enums for scene/loading/transition phases, eight camera modes, 20 player states, GameUI/dialog modes, and presentation selections. Preserve switch values and invalid-value behavior; add exhaustive-handling tests where native dispatch permits it.

Do not collapse distinct native states merely because their current implementations look similar.

**Exit:** state transitions read symbolically, serialized values are unchanged, and all state-machine suites pass exactly.

### Session 37 — Enemy, trigger, command, and content constants

Name enemy AI/action states, variant IDs, pool capacities, trigger types, interaction modes, GameSI command IDs, item/sound/animation indices, event-flag ranges, door special values, and gameplay sentinels. Use table-driven command registration where it removes duplication without changing dispatch behavior.

Special enemy properties belong in named per-class constants or typed configuration fields, not one global collection of unrelated numbers.

**Exit:** game logic and dispatch tables are self-describing, with exact command/state/persistence tests unchanged.

### Session 38 — Intent-revealing aliases and serialized-layout types

Add lightweight aliases where they clarify units and ownership, such as fixed-point values, angles, item/enemy/sound/command IDs, native addresses, and frame counts. Prefer aliases over elaborate strong-type machinery unless conversions are a demonstrated source of defects.

Define explicit serialized record types and field names separate from runtime objects. Add size/offset assertions where layouts are native-contractual, while retaining checked stream IO rather than raw structure reads.

**Exit:** public signatures communicate units and ID domains more clearly without ABI, save-byte, or arithmetic changes.

### Session 39 — Resolve unknown, unused, reserved, and padding fields

For every remaining unknown/unused field, enumerate all native reads/writes, constructors, vtable consumers, save/load code, and resource deserialization. Classify it as semantic state, cached/transient state, reserved, padding/alignment, or genuinely indeterminate. Check shadowed globals and adjacent-field overwrites before declaring padding.

Remove a portable field only when layout and serialization prove it is unnecessary; otherwise rename/comment it conservatively. Update Ghidra structures and exact save fixtures.

**Exit:** every field has a consumer-backed disposition; no “padding” claim rests only on absence of portable callers.

### Session 40 — Module ownership, deduplication, and large-file decomposition

Enforce the dependency direction `app → seven_days_game → soft3d → platform interfaces`, with no reverse game dependency. Move generic algorithms out of game code and episode rules out of engine/platform code only after native ownership is clear.

Deduplicate functions only when semantics, edge cases, and ownership match; preserve documented original duplicate compiled bodies in the native inventory. Split oversized files such as `GameSI.cpp`, `Units.cpp`, and `MaidBully.cpp` along existing subsystem boundaries without changing symbols' behavior.

**Exit:** include/dependency checks pass, no duplicate portable implementation remains without justification, and all tests and representative frames are unchanged.

### Session 41 — Configurable logical framebuffer and application presentation size

Centralize native 320×240 dimensions, strides, coordinates, projection aspect, UI placement, and buffer allocation behind explicit constants. Add guarded compile-time width/height overrides with 320×240 defaults; audit every hardcoded coordinate to classify it as framebuffer-relative, native UI data, world math, or an unrelated coincidental number.

Treat nonnative sizes as an experimental host variant, not pixel parity. Keep application window scaling and post-processing separate from the engine's logical framebuffer. Add default exact-pixel tests plus smoke/bounds tests for at least two nonnative sizes.

**Exit:** default output is byte/pixel identical, alternate sizes build and run without buffer errors, and any content still inherently fixed to 320×240 is explicitly documented.

### Session 42 — Test hygiene and refactor completion gate

Replace fixed temporary filenames with unique per-test/per-run directories so independent tests and build variants can run safely without collisions; retain serial execution only where a real shared resource requires it. Add dependency/layer checks and warnings-as-errors for the reconstructed targets where supported.

Re-run all variants, replay representative human scenarios, inspect generated artifacts, and compare public API/save/hash baselines. Update build documentation for every new option.

**Exit:** the refactor gate passes, test isolation is demonstrated, and no generated logs/saves/build products are added to source control.

---

## Phase F — Source comments, documentation, and Ghidra reconciliation

Comments are evidence summaries, not decompiler prose. Do not repeat what a line plainly does. Document contracts, units, ownership, native quirks, non-obvious arithmetic, and uncertainty. Every comment session must correct discrepancies found in source, docs, and Ghidra rather than choosing one silently.

### Session 43 — Comment convention, copyright plates, and core modules

Define the project's Google-C++-style comment convention in `Reversed_Game/README.md` or a short linked style section. Add module descriptions and the requested original provenance plate:

```cpp
// Copyright (c) 2006-2009, DingooGames. All rights reserved.
```

Keep reconstruction/authorship notices separate from the original copyright statement and do not invent a new license. Apply the convention to base/common/IO/log/resource modules first. Document public function purpose, arguments, returns/errors, ownership, and side effects in headers; comment complex implementation decisions at the point of interest.

**Exit:** all scoped core files have consistent plates/module/API comments and no comment contradicts current behavior.

### Session 44 — Math, formats, models, scripts, and stream comments

Document fixed-point formats/ranges/angle units, matrix order, overflow behavior, lookup tables, file structures, sentinel/endian rules, SOJ/SAI/STX/SBP/SPL/SDT/SST layouts, and VM task/call semantics. Include native addresses where they materially aid auditing, but keep long evidence tables in `Reverse_Docs`.

**Exit:** public APIs and complex parser/VM paths have argument/usage contracts and precise native quirks documented.

### Session 45 — Renderer, effects, audio, input, and generic UI comments

Document render queues/state selectors, clipping/depth/blending, scanline assumptions, BSP/PVS/lightmaps, effect ownership, shader history/orientation, mixer voice rules, positional audio, input transitions, SDT interpolation/blitter modes, and menu/configuration behavior.

**Exit:** every non-obvious algorithm has a concise explanation and each public method family has complete argument/return/ownership comments.

### Session 46 — Scene, camera, player, and GameSI comments

Document scene/day/map lifecycle, update/render ordering, door transitions, camera modes/restoration, player states and attack/QTE windows, and the GameSI context/dispatch contract. For command handlers, explain argument meaning and special return/sentinel behavior without duplicating the entire `gamesi_matrix.md` table in source.

**Exit:** a maintainer can follow startup, ordinary gameplay, transition, and script dispatch from source comments plus linked docs.

### Session 47 — Map units, triggers, enemies, and data-table comments

Comment interaction state machines, trigger persistence, Things/trap/stay-box behavior, common enemy AI, every variant override, linked actors, and boss phases. Fully annotate large tables—especially enemy constructor/configuration rows—with field name, type, units, valid range, sentinel, native source, and per-row exceptions.

Use column-group comments or a typed schema instead of repeating an opaque legend on every row. Confirm every claimed field meaning against consumers and Ghidra.

**Exit:** no major shipped table is a block of unexplained numbers and every special-enemy constant has a native-backed explanation.

### Session 48 — Save, license, debug, platform, and application comments

Document every save section/tag/link and writer/reader asymmetry, license protocol/policy distinctions, original debug-console behavior and opt-in safety additions, backend ownership, SDL orientation/presentation, and command-line options.

Distinguish original behavior, portable hardening, test policy, and application-only enhancements explicitly.

**Exit:** persistence/security-sensitive/platform code has complete contracts and no host-only policy is described as native behavior.

### Session 49 — Documentation and Ghidra final reconciliation

Sweep all `Reverse_Docs`, root `README.md`, and `Reversed_Game/README.md`. The root README currently lags the reconstruction status; update it and all stale stage-14/remaining-work statements. Reconcile counts, map totals, format statuses, controls, options, test commands, known limits, and intentional differences. Preserve historical reports with dated correction notes where useful.

In Ghidra, resolve inaccurate names/types/comments/tags/fields discovered during the roadmap, add missing native evidence, save, and read back unsaved status. Produce a final gap list containing only genuinely unverifiable or intentionally excluded items.

**Exit:** code, docs, tests, and Ghidra tell the same story; no stale completion claim or undocumented known discrepancy remains.

---

## Phase G — Developer build and investigation tools

### Session 50 — Developer configuration and real logging/performance output

Add a default-off developer CMake configuration/preset that enables appropriate warnings/assertions, STDIO logging, debug tooling, and test instrumentation without changing the faithful default. Add an independent stderr logging sink for Linux, including checked UTF-16-to-UTF-8 conversion and flush/error behavior; allow file, stderr, or both without reopening the file for every message in developer mode.

Keep native `--show-fps` restoration separate from a new, working application performance overlay, because the original helper's real-FPS field is not updated in this frame path. Expose the recovered debug console through an explicit developer/runtime option rather than a normal-build cheat shortcut.

**Exit:** default and developer builds both pass their suites, stderr output is readable and tested, the console remains opt-in, and the real FPS/frame-time overlay is not confused with native behavior.

### Session 51 — Interactive debugging and inspection controls

Add developer-only pause, single-frame advance, speed control, screenshot/state dump, replay capture, script/task inspection, and optional collision/trigger/waypoint overlays. Reuse the native console for health/item/day/map cheats; keep new inspection UI in the app layer where possible.

Any engine visualization hook must be compiled out or inert in faithful builds and must not mutate simulation state. Test debug jumps during door transitions and resource teardown under ASan/UBSan.

**Exit:** a developer can capture, reproduce, pause, inspect, and resume a failure from the same build without changing normal gameplay behavior.

---

## Phase H — Optional remake/remaster polish

These are recommendations, not parity requirements. Implement only after the relevant release-candidate gate. Default all enhancements off and label modified behavior in UI/configuration. Prefer application post-processing, input mapping, and resource-provider hooks over changes to simulation code.

### Session 52 — Emulator-style presentation filters and capture

Add an application-level post-processing chain after RGB565-to-XRGB conversion: nearest/bilinear scaling, integer-preserving filters such as Scale2x where practical, optional scanlines/CRT-style effects, aspect-correct scaling, and clean screenshot capture. Keep the original framebuffer available for exact tests and debugging.

**Exit:** filters work in both orientations without modifying engine pixels, default presentation is unchanged, and screenshots can be raw or filtered by choice.

### Session 53 — Modern controls and accessibility options

Add remappable keyboard/gamepad controls, configurable dead zones, pause behavior, and application-level display options. Consider optional text-speed, QTE-assist, hold-versus-toggle, and difficulty/accessibility aids only behind explicit non-parity settings; keep original 17 FPS simulation and default controls intact.

**Exit:** all options round-trip separately from native save/config data, defaults match current behavior, and assists clearly mark a modified gameplay session.

### Session 54 — Save/session management and player conveniences

Add application-level save-slot metadata previews, backup-before-overwrite, copy/delete with confirmation, replay/screenshot association, and optional autosave/checkpoint policy. Never change native slot bytes for these features; store host metadata separately.

**Exit:** malformed/host-metadata absence does not damage native saves, destructive actions are confirmed, and original slots remain interoperable with the reconstruction.

### Session 55 — Modding and content-inspection hooks

Add an opt-in loose-file/resource-override provider or mount layer above SPK lookup, plus developer viewers for models/animations, maps/entities, SDT tables, and SST tasks. Preserve archive fallback and checked paths; do not modify archived original resources.

**Exit:** a content author can override a resource or inspect shipped content without rebuilding the engine, while the default archive path and all fixture hashes remain unchanged.

---

## Recommended execution order and review checkpoints

1. **Sessions 29–34 and 34A first:** close the useful uncalled Soft3D and game-object API surfaces before extensive revalidation, so newly recovered code is audited only once. This maintainer-directed ordering supersedes the original Phase-D placement; it does not waive any phase gate.
2. **Sessions 01–03 next:** make the first-room failure reproducible and fix it before broad behavioral work.
3. **Sessions 04–21:** complete the ARM revalidation, including the APIs recovered in Phase D. Human exploratory reports may reprioritize a subsystem, but do not declare completion early.
4. **Sessions 22–28:** prove completability and all content with humans; fix nonblocking correctness issues before structural churn.
5. **Sessions 35–42:** perform behavior-preserving cleanup against the strengthened regression suite.
6. **Sessions 43–49:** comment and reconcile all artifacts; continue small documentation corrections in every earlier session as well.
7. **Sessions 50–51:** formalize the developer build after normal behavior is stable.
8. **Sessions 52–55:** choose optional polish individually; none is required for a faithful release.

The core roadmap ends at Session 51, with Sessions 52–55 optional. The most important scheduling rule is to avoid mixing categories: a correctness patch proves native behavior, a refactor proves unchanged behavior, and an enhancement proves that the default remains faithful.
