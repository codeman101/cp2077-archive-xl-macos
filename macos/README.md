# ArchiveXL for macOS (Apple Silicon)

A port of [psiberx/cp2077-archive-xl](https://github.com/psiberx/cp2077-archive-xl) (1.27.4: the v1.27.4 tag plus upstream b933859, the dynamic material fix the 1.27.4 release notes name) to the native Steam build of Cyberpunk 2077 for
Apple Silicon (game 2.3.1, arm64), built from source. The Windows build 20383525 is the reference for every address and layout. Port by
codeman101 (skyman3215). MIT, like upstream. It replaces the closed NightCity Console build (1.26.1), which loads no `.xl` files and has most of
its hooks inactive.

This file describes the Mac layer: `macos/`, the `__APPLE__` branches in `src/` and `lib/`, and `tools/mac/`.
`macos/WINDOWS_CHECK.md` lists every shared file that differs from upstream, for review before a Windows build.

Since 1.27 the built-in garment hide tags (`hide_Head`, `hide_Torso`, ...) live in ArchiveXL's bundle file `Bundle/VisualTags.xl` instead of the
code. It is plain YAML, so the Mac release ships it next to the dylib (`red4ext/plugins/ArchiveXL/Bundle/VisualTags.xl`). The rest of the bundle
needs `ArchiveXL.archive` (packed on Windows with WolvenKit) and is not shipped, as before. 1.27 features checked on the Mac: collision shape
deletions (CollisionNode shapes 0x48 and presets 0x98, the collision node loader Win 0x2ACDD8 = Mac 0x3321494) and the nails color fallback
(CharacterCustomizationSystem state 0x78, arms groups 0x90, CONFIRMED in `ADDRESS_CHECK.md`) are on.

## What works

Seen in menu boots of the release build; `INGAME_TESTS.md` names the log line that proves each feature and says what still needs a loaded world.

| area | state |
|---|---|
| Mounting the loose archives in `archive/Mac/mod` (the Mac game does not do it by itself), `mac-mount-skip.txt` | works |
| `.xl` discovery and loading (`EquipmentEx.archive.xl`, `LizziesBDs.archive.xl`, body mod `.xl` files) | works |
| Localization: onscreens, subtitles, voiceover and lipsync maps | works (about 61,500 onscreen entries merged with Equipment-EX and Lizzie's) |
| Factories (`FactoryIndex`), journal entries, mappin lookups, quest phases, world streaming blocks | registered at the menu; quests and streaming blocks run in game |
| Puppet state records, resource patches (EBB, VTK body mods), resource links and copies, mesh extension, appearance suffixes | works at the menu |
| Garment and attachment hooks (Equipment-EX outfit slots, transmog, third person), animation hook, character customization, entity spawner patch | hooks installed and counted; they act on dressed puppets, so the real proof is in game |
| World widget limit patch (more than 10 world space widgets) | applied at boot |
| Hot reload (`ArchiveXL.Reload()`) | works (diagnostic trigger) |

## Requirements

- macOS 14 or newer, Apple Silicon, Xcode command line tools, CMake 3.21+, Homebrew `spdlog`, `fmt`, `yaml-cpp`.
- The game 2.3.1 (Steam, native Mac build), RED4ext-macos with the Frida gadget, and the Mac build of Codeware from `cp2077-codeware` (ArchiveXL
  asks it for the engine, the game instance and the runtime scene, and the two plugins chain their hooks on the shared targets).
- The sibling checkout `../RED4ext.SDK-macos` on its `mac-full` branch (or `-DRED4EXT_SDK_DIR=<dir>`).

## Build

From the parent folder of the three repos: `tools/build-all.sh` (builds all three plugins, signs them, writes sha256 sums). For this repository alone:

```sh
cmake -S macos -B macos/build -DCMAKE_BUILD_TYPE=Release -DRED4EXT_SDK_DIR=../RED4ext.SDK-macos
cmake --build macos/build -j
```

The post-build steps rewrite the install names of `libfmt`, `libspdlog` and `libyaml-cpp` to `@loader_path` and fail the build when the dylib
needs a symbol of the ArchiveXL namespaces that it does not define. Install `ArchiveXL.dylib` with its three libraries into
`red4ext/plugins/ArchiveXL/`. The scripts in `scripts/` go into the game's script cache with the other script mods
(`cp2077-codeware/tools/build-cache.py --plugin-scripts ArchiveXL=<this repo>/scripts`).

## Mount skip list

The Mac game does not mount `archive/Mac/mod`, so ArchiveXL mounts every `*.archive` there (byte order, the same rule as the game's own archive sort; first file wins; `.archive.off` is
ignored). Archives that the CP2077 Mac Archive Patcher already baked into the official archives must not be mounted a second time: list their
file names in `mac-mount-skip.txt` next to `ArchiveXL.dylib` (template: `macos/mac-mount-skip.example.txt`). The `.xl` file next to a skipped
archive is still found.

## Runtime hooks and the allowlist

RED4ext-macos installs a detour only on offsets listed in `RED4EXT_GUM_HOOK_OFFSETS`. For any other offset `Attach` reports success and hands
back the target itself as the original, which is a silent no-op. `macos/hook-offsets.txt` lists the offsets ArchiveXL hooks (about 90, two of
them conditional: the garment package patch is attached only when a loaded `.xl` patches an entity template); `tools/build-all.sh` merges it
with the other two plugins' lists into `GUM_OFFSETS.txt`. Four targets are shared with Codeware (`0x2f67db4`, `0x3d9e494`, `0x3d9efc8`,
`0xc95744`); the plugins chain on them (`macos/MacHookChain.cpp`). Address rows are enabled one module at a time in `macos/MacEnabledRows.inc`;
every other row resolves to a trap stub that logs `[axl] not on Mac yet: ...` and returns zero, and the hooking driver refuses to hook a trap.

## Environment variables

| variable | effect |
|---|---|
| `CODEWARE_MAC_DIAG=1` | diagnostics and self-tests (the flag is shared with Codeware). Default: off, only normal log lines and errors. |
| `ARCHIVEXL_MAC_GARMENT_JOB=0/1` | force the garment package patch hook off or on (default: attached only when an `.ent` patch target exists). |
| `ARCHIVEXL_MAC_NOMOUNT=1` | do not mount `archive/Mac/mod` (probe boot for the depot test; diagnostics). |
| `ARCHIVEXL_MAC_RELOAD_TEST=1` | with diagnostics: call `ArchiveXL.Reload()` at t+50 s. |

## Mac ABI notes

Same rules as Codeware (see `cp2077-codeware/macos/README.md`): `this` in x0, non-trivial results through x8 (the Windows `T& aOut` argument
disappears), Mac vtable slot = Windows slot + 8, overloaded virtuals in the opposite order, the game's `SharedSpinLock` protocol (bit 7 is the
writer). `macos/MacLayoutGuard.cpp` (generated by `tools/mac/gen-mac-layout.py`) asserts the offsets of every SDK member the plugin touches;
`macos/MacPools.inc` (generated by `tools/mac/gen-pools.py`) resolves the game's memory pools by their exported names.
`tools/mac/check-sdk-layout.py` checks every class of the SDK (not only those ArchiveXL sees) against the reflection dump: run it after a change of the SDK headers.

## Known gap: RED4ext's own exit code crashes (worked around in the plugin)

The installed RED4ext.dylib (NightCity Console build) crashes at the end of its own exit code on every normal quit: App::Destruct opens a DetourTransaction, does not commit it, and the destructor logs through an spdlog default logger that is already null (RED4ext.dylib+0xdb134; the same stack is in the 2026-10-03 crash reports, before any of our plugins). On Apple the last of our three plugins to unload leaves the process with _exit(0), only while the process is exiting (the Unload call must be inside libsystem_c's `exit()`, checked on the stack with backtrace and dladdr), after flushing its logs. Mac only (macos/MacExit.hpp). A plain Unload (reload, failed load, one plugin alone) never exits.
The plugin's `Main` (Unload) also leaves its application and extension objects alone (`g_app.release()`): they hold tokens and handles of the game, and
destroying them from RED4ext's atexit handler went through freed SDK relocation tables (the r27 quit crash, fixed in the SDK as well). Environment keys:
`CP2077_MAC_PLUGIN_AXL_STATE`, `CP2077_MAC_PLUGIN_CW_STATE`, `CP2077_MAC_PLUGIN_TXL_STATE` ("loaded" / "unloaded").

## Files

- `macos/NOT_YET.md`: address hashes without a Mac address (72 traps), the Mac gaps ArchiveXL fills itself, and upstream findings. Generated by
  `tools/mac/gen-not-yet.py` from `tools/mac/not-yet-notes.md`.
- `macos/ADDRESS_CHECK.md`: every address row, vtable slot and field offset checked against the Windows twin.
- `macos/INGAME_TESTS.md`: what only fires in game, with the log line that proves it, and the end-test steps.
- `macos/hook-offsets.txt`: the allowlist offsets of this plugin. `macos/MacEnabledRows.inc`: the rows enabled on Mac.
- The `[selftest] R22 ...` lines come from `src/App/Utils/MacProbe.cpp` and the modules' `LogMacStats` functions and run only with
  `CODEWARE_MAC_DIAG=1`.
