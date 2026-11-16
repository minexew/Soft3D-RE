# SST simple scripts and the Symbian VM

## Scope and evidence

SST is the compiled form of the game's “simple script” language. The Mobile/Symbian variant contains typed variables, an optional initializer, named-by-index task functions, recursive commands, and a compact tree-based VM. It is not a byte-coded stack machine.

The layouts and behavior below come from the ARMv5 Symbian 3rd binary, especially:

- `ScriptInterface` constructor/dispatch at `0x00130780` and `0x00130a88`;
- command reader, continuation linker, and executor at `0x00131370`, `0x001318d8`, and `0x00131c34`;
- function reader/start/step at `0x00132630`, `0x00133108`, and `0x0013318c`;
- `SimpleScript_LoadFromPak`, `SimpleScript_Start`, and `SimpleScript_Update` at `0x00134b58`, `0x00135948`, and `0x001359e4`;
- argument reader and typed operation evaluator at `0x001362f0` and `0x001364c0`.

`Reversed_Game/src/soft3d/SimpleScript.*` implements the portable parser, VM, task lifecycle, and generic host contract. `GameSI`'s episode-specific command handlers remain outside the engine layer.

## Mobile binary format

All integer fields are little-endian. Unless stated otherwise, counts and tags are unsigned 16-bit words. There is no file signature or version header.

### Top level

| Order | Field | Meaning |
|---|---|---|
| 1 | `uint16 globalCount` | Number of script-global variables |
| 2 | `uint16 globalTypes[globalCount]` | One declared type per global |
| 3 | `uint16 initializerPresent` | Nonzero means an initializer function follows |
| 4 | `Function initializer` | Present only when `initializerPresent != 0` |
| 5 | `uint16 functionCount` | Number of task functions |
| 6 | `Function functions[functionCount]` | Tasks addressed by zero-based index |

The original engine has room for 256 functions. Its shared pools hold 1600 commands and 3000 values. The portable parser enforces these capacities and rejects truncation, unknown types, excessive arguments, invalid references, and trailing data instead of reading outside the input.

### Function record

| Order | Field | Meaning |
|---|---|---|
| 1 | `uint16 localCount` | Number of function-local variables |
| 2 | `uint16 localTypes[localCount]` | Declared local types |
| 3 | `uint16 entryCommandCount` | Entry/enable-condition command list |
| 4 | `Command entryCommands[...]` | Usually zero or one command |
| 5 | `uint16 commandCount` | Primary body command count |
| 6 | `Command commands[...]` | Primary instruction tree |

Entry commands are evaluated from their first record whenever an enabled task is considered. A null result starts the primary body; a non-null result leaves the task enabled and waiting. An empty entry list starts immediately.

### Command record

| Order | Field | Meaning |
|---|---|---|
| 1 | `uint16 id` | Internal opcode or host-command ID |
| 2 | `uint16 type` | Execution cadence: 0 yields after execution, 1 continues |
| 3 | `uint16 argumentCount` | Up to 32 arguments |
| 4 | `Argument arguments[argumentCount]` | Typed literals or variable references |
| 5 | `uint16 resultType` | Normally 36 when unused; 6 means a reference result follows |
| 6 | `uint16 resultSize, resultIndex, resultScope` | Present only for result type 6; real files use size 4 |
| 7 | `uint16 bodyCount` | Nested primary/body commands |
| 8 | `Command body[bodyCount]` | Recursive body list |
| 9 | `uint16 conditionCount` | Nested expression/condition commands |
| 10 | `Command conditions[conditionCount]` | Recursive condition list |

Each argument starts with signed 16-bit `type` and `size` fields, followed by `size` bytes. Non-string payloads are four-byte words. String payloads are narrow ASCII in the file and converted to UTF-16 by the engine. The original cursor advances by `size & ~1`; real string records have even sizes and include a terminating zero.

### Value and reference types

| ID | Type | Runtime representation |
|---|---|---|
| 0 | byte | Zero-initialized 32-bit storage; byte accessors narrow it |
| 1 | short | Sign-extended/narrowed through 16-bit accessors |
| 2 | int | Signed 32-bit word |
| 3 | string | Owned UTF-16 buffer |
| 4 | bool | 32-bit zero/nonzero word |
| 5 | fixed | Signed 16.16 fixed-point word |
| 6 | reference | Serialized reference; resolved to another value record |

A type-6 argument packs the zero-based variable index into the low 16 bits and its scope into the high 16 bits. An output record stores the same information as separate `resultIndex` and `resultScope` words.

| Scope | Target |
|---|---|
| 0 | Scope-0 array at `SimpleScript + 0x34b4`; no real-script use was found |
| 1 | Script globals |
| 2 | Currently executing function's locals |

Scope 2 is dynamic. A called task body does not permanently capture its owning function's locals. This is essential to `runTask` behavior.

## Internal opcodes

IDs 0–6 and 15 have no implemented dispatch in the recovered executor. Real shipped scripts use the following subset of internal IDs, but the reconstruction covers every implemented ID.

| ID | Operation | Behavior |
|---|---|---|
| 7 | `if` | Evaluate condition children; true enters body, false uses next sibling |
| 8 | `switch` | Evaluate conditions, then integer-compare argument 0 with each body child's argument 0 |
| 9 | `wait` | Evaluate conditions; true advances, false returns the same command |
| 10 | `return` | Reset the owning function and terminate the current stream |
| 11 | `else` | Enter body |
| 12 | `case` | True enters body; false advances to the next case/sibling |
| 13 | `while` | Evaluate conditions; true enters body, false advances |
| 14 | terminate | Return null without directly resetting the owning function |
| 16–21 | `== <= >= < > !=` | Typed comparison into result reference |
| 22, 23 | `&&`, `||` | Boolean truth test into result reference |
| 24 | `!` | Boolean negation for integer and bool inputs |
| 25 | assignment | Copy argument 0's payload into result without changing result type |
| 26–30 | `+= -= *= /= %=` | In-place operation on result and argument 0 |
| 31–35 | `+ - * / %` | Binary operation on arguments 0 and 1 into result |
| 36 and above | host command | Dispatch through `ScriptInterface` |

Only `if`, `switch`, `wait`, and `while` execute their condition-child lists. Body and condition commands are stored as separate recursive lists.

The left operand's type selects arithmetic/comparison behavior. Integer operations use 32-bit ARM wrapping; fixed operations use the recovered 16.16 helpers. Strings support comparisons only. Boolean and fixed values can participate in logical operations through zero/nonzero tests. Fixed-point `%`/`%=`, string arithmetic/logic, and fixed `!` do not write a result. Output records retain their declared type even when written by a Boolean comparison.

### Continuation links

Each body command has a next-sibling pointer and a continuation pointer. The loader computes continuations after parsing:

- `if` bodies continue after the `if`, skipping an immediately following `else`;
- `else` bodies continue at the else's successor or inherited continuation;
- `while` bodies continue at the loop command itself;
- `case` bodies skip sibling cases and continue after the switch or at the inherited continuation.

A null executor result falls back to the continuation. Commands are numbered in post-order after their children. The ordinal-based lookup helper at `0x001321bc` has an original recursion bug: it recursively calls itself on the same node and never advances through the body array. The portable VM does not depend on that helper.

## Generic `ScriptInterface`

The base interface owns seven direct callbacks. `ScriptInterface_ExecuteCommand` handles IDs below 43 through this table and forwards higher IDs to virtual slot 2, implemented by `GameSI` in the game.

| ID | Generic callback | Effect |
|---|---|---|
| 36 | `runScript(string)` | Clear shared words, load the named SST, and start its initializer |
| 37 | `enableTask(short)` | Disabled → enabled; suspended → running |
| 38 | `disableTask(short)` | Enabled → disabled; running → suspended |
| 39 | `runTask(short)` | Borrow another function's primary stream into the current task |
| 40 | `intermitTask(short)` | Force-reset/disable the target task |
| 41 | `setVar(short, int)` | Write one of 256 shared 32-bit words |
| 42 | `getVar(short)` | Write a shared word to the command's result reference |

The shared-word array is separate from typed script globals. The reconstruction exposes the same contract through `ScriptInterface::Execute` and a protected `ExecuteGameCommand` override for higher IDs; it does not implement `GameSI`.

## Task execution and calls

Function states are:

| State | Meaning |
|---|---|
| 0 | disabled |
| 1 | enabled; entry condition may be evaluated |
| 2 | running |
| 3 | suspended; `Update` still calls `Step`, while enable/disable transitions remain distinct |

A normal function reset restores the primary cursor, clears the borrowed call cursor, and clears local payloads. Running becomes enabled; suspended becomes disabled. Forced interruption always disables. Ordinary body completion therefore leaves a task enabled for future entry evaluation.

`runTask` stores only a target task index and a borrowed command cursor in the caller. It does not create a stack frame or switch the current variable context. The called body begins on a subsequent `Step`, and its completion clears the caller's local payloads and resets the called-task index. A type-0 command yields after execution in the primary stream. Type 1 continues. A false type-1 `wait` can therefore retry within the same step indefinitely; real scripts use type 0 for ordinary suspending waits. Called streams additionally stop after `wait` commands.

## `SimpleScript` lifecycle

Script-level states observed in the ARM implementation are:

| State | Meaning |
|---|---|
| 0 | unloaded/cleared |
| 1 | loaded |
| 2 | initializer running |
| 3 | initializer complete |
| 4 | task functions running |

`LoadFromPak` appends `.sst`, reads the resource, builds the pools, and enters state 1. `Start` resets functions and enters state 2. `Update` is a no-op without a `ScriptInterface`.

State 2 runs the initializer. If the current command's post-order ordinal is at most 2, the original performs only one `Step`; otherwise it loops until a step completes. It then enters state 3. State 4 iterates functions in index order, updating running/suspended functions and evaluating enabled entry conditions. A newly enabled later-index function can run in the same update.

`Reset` clears shared words and value payloads and resets task cursors. `Unload` releases the parsed script in the portable implementation. The original also has archive-state readers/writers at `0x00135268`, `0x00135560`, and helpers nearby; their generic stream/state integration belongs to stage 12, not the SST instruction parser.

### Portable safety differences

The reconstruction preserves observable VM behavior but uses owned C++ objects instead of native pointers into fixed ARM arrays. It validates input bounds, counts, types, reference scopes, and task/shared-array indices. Integer division/modulus by zero return zero rather than relying on a host ABI trap. A `runScript` requested during execution is deferred until the update finishes so that the active command tree is not destroyed during its own callback; the original immediately reuses its fixed pools.

## Win32 variant and tools

Win32 SST is a different serialization, not a narrower version of the Mobile records. The existing C# Win32 reader remains responsible for its byte-sized command headers, byte-sized argument tags, brace-line metadata, and optional command conditions. The Mobile reader and decompiler were corrected independently from the ARM evidence above; no Mobile layout is imposed on Win32 files.

`Tools/SimpleScriptDecompiler` now names Mobile body/condition/result fields accurately, supports all declared types and reference operands, enforces the 32-argument layout, and handles every recovered internal opcode. Verification covered all 217 shipped Mobile SST files with zero parser/decompiler failures. The C++ tests additionally load real scripts directly from `res.spk`, check exact resource counts, and execute directed synthetic scripts for every internal and generic opcode, all value types, branches, loops, calls, waits, termination, reset, and reload paths.

## Key runtime layouts

| Structure | Size | Important offsets |
|---|---:|---|
| `ScriptValue` | `0x10` | type `0x0`, data `0x4`, size `0x8`, flags `0xc` |
| `ScriptCommand` | `0xb4` | arguments `0x8` (32 pointers), output `0x88`, body `0x8c/0x90`, function `0x94`, script `0x98`, type `0x9c`, next `0xa0`, continuation `0xa4`, ordinal `0xa8`, conditions `0xac/0xb0` |
| `ScriptFunction` | `0x30` | state `0x0`, commands `0x4/0x8`, current `0xc`, entry `0x10/0x14`, called task/command `0x18/0x1c`, script `0x20`, locals `0x24/0x28`, total commands `0x2c` |
| `ScriptInterface` | `0x40` | generic callback pairs `0x4..0x3b`, script back-pointer `0x3c` |
| `SimpleScript` | `0x34dc` | state `0x28`, functions `0x30/0x34`, initializer `0x3034/0x3038`, interface `0x306c`, shared words `0x30b4`, globals `0x34bc/0x34c0`, current locals `0x34c4/0x34c8`, pool usage `0x34cc/0x34d0`, pools `0x34d4/0x34d8` |
