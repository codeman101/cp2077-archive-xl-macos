# Windows check for ArchiveXL (mac-full against upstream b933859: the v1.27.4 tag 5474e34 plus the dynamic material fix)

This file lists the 100 shared source files outside `macos/`, `tools/mac/` and documentation that differ between upstream `b933859` (R31 merge, 2026-10-09) and the mac-full tip at the time of writing, and says whether each change can affect the Windows build or Windows behaviour. It is a static comparison (Windows cannot be built here): each file was reduced to the text a Windows compiler sees (`__APPLE__` undefined, `_WIN32` defined) and compared with upstream, and every file with a remaining difference was read by hand. Result: 59 modified files are fully Apple-guarded, 1 is an ignore-rule change, 6 new files are unreachable from the Windows build or reduce to the old include, and 34 changes are unguarded, of which 33 are NO-OP and 1 is a CHECK item that only needs a Windows build to confirm.

## Summary table

| Measure | Files |
|---|---|
| Shared files changed outside macos/, tools/mac/ and documentation | 102 |
| Modified, Apple-guarded only (Windows-visible text identical to upstream) | 60 |
| Modified, non-code (ignore rules only) | 1 |
| Additions only (not compiled on Windows, or reducing to the old include) | 7 |
| Unguarded changes (modified files and compiled additions) | 34 |
| of which NO-OP (same behaviour on Windows) | 33 |
| of which CHECK, low risk (needs a Windows build to confirm) | 1 |
| of which CHECK, breaks or may break the Windows build | 0 |

## Shared files changed

NONE: every hunk is inside an `__APPLE__` branch, or a `_WIN32`/`#else` split whose Windows branch is the upstream text, or the file is new and unreachable from the Windows build. NO-OP: unguarded, but the same behaviour on Windows (reason given). CHECK: unguarded behaviour or compile change that Windows CI should build and review.

| file | kind | Windows impact | one-line reason |
|---|---|---|---|
| `.gitignore` | modified | NONE | Adds `macos/build/`, `*.rej` and `*.orig` ignore rules; no build input. |
| `lib/Core/Facades/Runtime.cpp`<br>`lib/Core/Facades/Runtime.hpp`<br>`lib/Core/Foundation/RuntimeProvider.cpp` | modified | NONE | Mac overloads (`IsASI(void*)`, `IsEXE(std::string_view)`, ctor taking `void*`) added in the `#else` of `_WIN32`; the Windows overloads are untouched. |
| `lib/Core/Foundation/RuntimeProvider.hpp`<br>`lib/Core/Runtime/ModuleImage.hpp`<br>`lib/Core/Runtime/OwnerMutex.hpp` | modified | NO-OP (risk: none) | `#include "Core/Win.hpp"` replaced by `Core/Platform.hpp`, which includes Win.hpp on Windows. |
| `lib/Core/Hooking/Detail.hpp` | modified | NONE | Apple-only early return in `HookResult` (a refused hook target logs and leaves the feature off instead of throwing); R26: Apple-only per-hook call counters (the include of `HookHits.hpp`, a call in `Attach`, a bump in `Handle`, a static counter member; every hunk is inside `#if defined(__APPLE__)`). |
| `lib/Core/Hooking/HookHits.hpp` | added | NONE | New (R26). The whole content is inside `#if defined(__APPLE__)`; only `Detail.hpp` includes it, and only on Apple. |
| `lib/Core/Logging/LoggingAgent.cpp` | modified | NONE | Apple-only guard so `LogFlush` works before a logging driver is set. |
| `lib/Core/Platform.hpp` | added | NONE | New. On Windows it only includes `Core/Win.hpp`, which is what the including headers did before; the `#else` includes macOS.hpp. |
| `lib/Core/Raw.hpp` | modified | CHECK (risk: low) | `RawVFunc` gets a defaulted `bool VerifiedOnMac` template parameter and an extra `{}` scope around the call, and the file includes Platform.hpp. Windows ignores the flag and keeps `offset = A`; the bindings in this repo that pass the flag do so only in Apple branches. |
| `lib/Core/Runtime/HostImage.cpp` | modified | NO-OP (risk: none) | The six getters (GetBase, GetPath, GetName, GetRootDir, GetFileVer, GetProductVer) moved below the platform `#if` so both platforms share them; bodies identical to upstream. |
| `lib/Core/Runtime/HostImage.hpp` | modified | NO-OP (risk: none) | Adds `#include "Core/Platform.hpp"` (Win.hpp on Windows). |
| `lib/Core/Runtime/ModuleImage.cpp` | modified | NONE | Mac implementation (dladdr) in the non-Windows branch. |
| `lib/Core/Runtime/OwnerMutex.cpp` | modified | NONE | Mac file lock in /tmp stands in for the named mutex, non-Windows branch. |
| `lib/Core/Stl.hpp` | modified | NONE | Mac std containers in `#else`; the TiltedPhoques and hopscotch aliases stay in the `_WIN32` branch with the same text. |
| `lib/Core/Win.hpp` | modified | NONE | Windows includes now sit inside `#if _WIN32` with the same text; the `#else` includes `Core/macOS.hpp`. |
| `lib/Core/macOS.hpp` | added | NONE | Mac platform helpers; reachable only from non-Windows branches (Platform.hpp, Win.hpp, HostImage.cpp). |
| `lib/Red/Engine/Framework.hpp` | modified | NONE | Apple-only declarations (Mac game-instance accessors). |
| `lib/Red/Specializations.hpp` | modified | NO-OP (risk: none) | `size_t` to `uint64_t` in the enum `std::hash` specialisation; the same type on Windows x64. R26: the `std::hash<TweakDBID>` hunk is inside `#if defined(__APPLE__)` (the Windows branch is the upstream line). |
| `lib/Red/TypeInfo/Definition.hpp` | modified | NO-OP (risk: none) | Apple-only changes (Mac global publishing, the `SystemBuilder` Mac variant, the "already registered" skip); R31: the type virtuals of `ClassDescriptorDefaultImpl` are spelled with `Detail::InstancePtr` / `ConstInstancePtr`, which are `void*` / `const void*` on Windows (upstream's types). Same text as Codeware's copy. |
| `lib/Red/TypeInfo/Macros/Definition.hpp` | modified | NO-OP (risk: none) | Appends `X_RTTI_FORCE_REGISTRAR(...)` to the RTTI_* macros; it is defined empty off Apple, so expansions are unchanged. |
| `lib/Red/TypeInfo/Registrar.hpp` | modified | NO-OP (risk: none) | Three unused static inline helpers (`RunPending`, `PendingRegisterCount`, `PendingDescribeCount`) and an empty `TraceCallback(const char*, Callback) {}` called before each queued callback (leftover trace hook, no effect). |
| `lib/Red/TypeInfo/Resolving.hpp` | modified | NO-OP (risk: none) | `IsScopedEnumName` becomes a template on `T` (T only matters in the Apple branch); the `"enum RED4ext::"` prefix and the result are unchanged; loop index typed `size_t` instead of `auto`. |
| `lib/Support/MinHook/MinHookProvider.cpp`<br>`lib/Support/MinHook/MinHookProvider.hpp` | modified | NO-OP (risk: none) | One comment line added. |
| `lib/Support/RED4ext/RED4extProvider.hpp` | modified | NO-OP (risk: none) | Adds `#include "Support/RED4ext/V1Compat.hpp"`, whose content is Apple-only. |
| `lib/Support/RED4ext/V1Compat.hpp` | added | NONE | `RED4ext::v1` aliases for the Mac SDK; whole content under `#if defined(__APPLE__)`. |
| `lib/Support/RedLib/RedLibProvider.cpp` | modified | NONE | Apple-only lifecycle include and queue handling. |
| `lib/Support/macOS/Lifecycle.hpp` | added | NONE | Mac lifecycle event dispatcher; included only from Apple branches. |
| `src/App/Application.cpp` | modified | NO-OP (risk: none) | The `RedLibProvider.hpp` include moved up by two lines; the Mac constructor is Apple-only; the Windows constructor equals upstream (v1 `Sdk`). |
| `src/App/Application.hpp` | modified | NONE | Apple-only constructor declaration. |
| `src/App/Archives/ArchiveService.cpp`<br>`src/App/Archives/ArchiveService.hpp` | modified | NONE | Apple-only mounting of the loose archive/Mac/mod folder (Windows mounts archive/pc/mod natively) and the mount-skip list. |
| `src/App/Extensions/Animation/Extension.cpp`<br>`src/App/Extensions/Attachment/Extension.cpp`<br>`src/App/Extensions/Attachment/Extension.hpp`<br>`src/App/Extensions/Customization/Extension.cpp`<br>`src/App/Extensions/ExtensionLoader.cpp`<br>`src/App/Extensions/ExtensionService.cpp`<br>`src/App/Extensions/Garment/Dynamic.cpp`<br>`src/App/Extensions/Garment/Dynamic.hpp`<br>`src/App/Extensions/Garment/Wrapper.cpp`<br>`src/App/Extensions/InkSpawner/Extension.hpp`<br>`src/App/Extensions/Journal/Extension.hpp`<br>`src/App/Extensions/Localization/Extension.cpp`<br>`src/App/Extensions/PuppetState/Extension.cpp`<br>`src/App/Extensions/PuppetState/Extension.hpp`<br>`src/App/Extensions/PuppetState/Handler.cpp`<br>`src/App/Extensions/QuestPhase/Extension.cpp`<br>`src/App/Extensions/Transmog/Extension.cpp`<br>`src/App/Extensions/Transmog/Extension.hpp`<br>`src/App/Extensions/WorldStreaming/Extension.cpp` | modified | NONE | Apple-only Mac hook variants for the inlined or re-shaped game functions (suffix handling, journal finders, spawn hooks, streaming blocks), self-test counters and Mac-ABI call shapes; Windows code paths unchanged. R27: `ExtensionService::OnShutdown` leaves the loader alone on Apple (`#if defined(__APPLE__)`, see README, RED4ext exit crash). |
| `src/App/Extensions/Animation/Extension.hpp`<br>`src/App/Extensions/Customization/Extension.hpp`<br>`src/App/Extensions/Garment/Extension.hpp`<br>`src/App/Extensions/Mesh/Extension.hpp`<br>`src/App/Extensions/ResourceLink/Extension.hpp`<br>`src/App/Extensions/ResourcePatch/Extension.hpp` | modified | NO-OP (risk: none) | Adds `#include <atomic>` (Mesh/Extension.hpp also declares `IsPreloadBlocked`, Apple-only since R31). |
| `src/App/Extensions/Garment/Extension.cpp` | modified | NO-OP (risk: none) | Adds `#include "App/Extensions/Attachment/Extension.hpp"`; the `Mac::Count` call in `OnGameDetach` is now inside `#if defined(__APPLE__)`. |
| `src/App/Extensions/InkSpawner/Extension.cpp` | modified | NO-OP (risk: none) | Braces added around a one-line `return;`. |
| `src/App/Extensions/Journal/Extension.cpp` | modified | NO-OP (risk: none) | `std::max(...)` becomes `std::max<size_t>(...)` with a plain `2` instead of `2ull`; same type (unsigned 64-bit) on Windows. R27: `ResolveMappinPosition` builds the `ScriptGameInstance` with the live instance under `#if defined(__APPLE__)` (the default constructor reads `CGameEngine::Get()`, which has no Mac address); the `#else` branch is the original line. |
| `src/App/Extensions/Mesh/Extension.cpp` | modified | NONE | R31: `OnPreloadAppearances` is upstream's 1.27 text again; `IsPreloadBlocked()` (the same rule, for the Mac preload action hook) and the counters are Apple-only. |
| `src/App/Extensions/ResourceLink/Extension.cpp` | modified | NO-OP (risk: none) | Adds `#include <cstring>`. |
| `src/App/Extensions/ResourcePatch/Extension.cpp` | modified | NO-OP (risk: none) | Both `Dispatch` lambdas capture a local copy `builderWeak` (a `WeakPtr<EntityBuilder>`, the member type) instead of copying `aParams->entityBuilderWeak` at each call; same weak pointer value. |
| `src/App/Patches/EntitySpawnerPatch.cpp` | modified | NONE | Apple-only counters and the null-request check; the Windows text equals upstream. |
| `src/App/Patches/EntitySpawnerPatch.hpp` | modified | NO-OP (risk: none) | Adds `#include <atomic>`. |
| `src/App/Patches/WorldWidgetLimitPatch.cpp` | modified | NONE | Apple-only arm64 patch of the world widget limit; the Windows patch is unchanged. |
| `src/App/Shared/ResourcePathRegistry.cpp` | modified | NO-OP (risk: none) | One comment line added. |
| `src/App/Shared/ResourcePathRegistry.hpp` | modified | NONE | Apple-only call counter for the shared ResourcePath::Create hook. |
| `src/App/Utils/MacProbe.cpp` | added | NONE | Mac diagnostic probe; the whole file is wrapped in `#if defined(__APPLE__)`, so the xmake `src/**.cpp` glob compiles an empty unit. |
| `src/App/Utils/MacProbe.hpp` | added | NONE | Declarations only (`App::MacProbe`), not guarded but nothing on Windows includes or calls them; it emits no code. |
| `src/App/Utils/Registers.hpp`<br>`src/App/Utils/Str.hpp` | modified | NONE | Apple-only: arm64 replacement for the x64 register helpers, UTF-32 `wchar_t` string widening. |
| `src/Red/Addresses/Library.hpp` | modified | NONE | Apple-only address ids for Mac-only hook targets. |
| `src/Red/AppearanceChanger.hpp`<br>`src/Red/AppearanceResource.hpp`<br>`src/Red/Common.hpp`<br>`src/Red/EntityBuilder.hpp`<br>`src/Red/FactoryIndex.hpp`<br>`src/Red/InkSpawner.hpp`<br>`src/Red/ItemObject.hpp`<br>`src/Red/JournalManager.hpp`<br>`src/Red/Localization.hpp`<br>`src/Red/MappinSystem.hpp`<br>`src/Red/QuestsSystem.hpp`<br>`src/Red/ResourceDepot.hpp`<br>`src/Red/ResourceLoader.hpp`<br>`src/Red/ResourcePath.hpp`<br>`src/Red/SharedStorage.hpp`<br>`src/Red/TPPRepresentationComponent.hpp`<br>`src/Red/VisualTagsPreset.hpp`<br>`src/Red/WorldNode.hpp` | modified | NONE | Apple-only Mac ABI variants and bindings (results through x8, no out argument, changed slots and argument lists, Mac-only functions); the Windows declarations are unchanged. |
| `src/Red/Buffer.hpp`<br>`src/Red/EntitySpawner.hpp`<br>`src/Red/GarmentAssembler.hpp` | modified | NO-OP (risk: none) | Adds `#include "Red/Common.hpp"` (an existing header); Buffer.hpp also gets an Apple-only `Clone` variant beside the unchanged Windows one. |
| `src/Red/CharacterCustomization.hpp` | modified | NO-OP (risk: none) | The six `RawFunc` bindings share a new `GetAppearancesFn` alias; the function type is the same as before. |
| `src/Red/Entity.hpp`<br>`src/Red/Mesh.hpp` | modified | NO-OP (risk: none) | A comment line, and the `RawVFunc` bindings get an Apple-only extra `, true` argument; on Windows only the closing `>()` moves to its own line (same tokens). |
| `src/Red/TweakDB.hpp` | modified | NONE | R26: `GetFlatPtr` drops the offset bytes of the id it is given, inside `#if defined(__APPLE__)`; the Windows lines are the upstream text. |
| `src/pch.hpp` | modified | NONE | Apple-only include of the ink animation types. |

## Remaining items for Windows CI

No known Windows compile break or behaviour change is open. A Windows CI build (xmake) is still needed to confirm one low-risk item:

- `lib/Core/Raw.hpp`. Risk: low. `RawVFunc` and its partial specialisations gain `bool VerifiedOnMac` after a parameter pack (deducible, so standard C++); Windows keeps `offset = A` and the old call body (now inside an extra `{ }`), so behaviour is identical, and the bindings in this repo that pass the flag do so only in Apple branches. Only compiler acceptance is open.

Everything else that still differs on the Windows side is rated NO-OP (risk: none): empty `X_RTTI_FORCE_REGISTRAR` macro, extra includes (`<atomic>`, `<cstring>`, `Red/Common.hpp`, `Attachment/Extension.hpp`), the `Resolving.hpp` template (`IsScopedEnumName` with an unused `T` off Apple), the `builderWeak` lambda capture, `std::max<size_t>` (R31: with upstream's `Size()` calls), the `Localization` lookup that returns `it.Base()` on Windows and the plain pointer on Apple (a `_WIN32`-equivalent split), and one leftover trace hook: the empty `TraceCallback(const char*, Callback)` in `lib/Red/TypeInfo/Registrar.hpp`, which copies each queued callback once and does nothing (can be removed for tidiness).
