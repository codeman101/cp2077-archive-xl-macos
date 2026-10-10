## Mac gaps that ArchiveXL fills itself

Things the Windows game does for ArchiveXL (or that Windows ArchiveXL relies on) and the Steam 2.3.1 Mac game does not.

1. **The Mac game does not mount `archive/Mac/mod`.** Windows mounts `archive\pc\mod` natively; the Mac depot holds exactly three archive
   groups (`archive/mac/content`, `ep1`, `patch`). A probe boot with ArchiveXL's mount disabled (`ARCHIVEXL_MAC_NOMOUNT=1`, diagnostics on)
   read `ResourceDepot::CheckResource` for Lizzie's journal, her quest phase and the first resource of `LizziesBDs.archive` at
   InitResourceDepot, after LoadScripts, after LoadGatheredResources and at t+50 s: always 0, and the group list never changed (3 groups, no
   scope Mod). The loose archives whose first resource answers 1 (weather, Citizen Breast Physics, Y_Sneaky EBB refits) answer 1 because the
   Mac Archive Patcher baked their records into the official archives. `App::ArchiveService` therefore mounts `archive/Mac/mod/*.archive`
   itself on macOS: a Mod-scope group (scope 4) inserted before the official groups, with the Mac `LoadArchives`
   (group, paths, loaded, memoryResident, mode; the depot argument is gone). The paths are sorted in ASCII order and the depot lookup takes the
   first archive that has a resource, so the first file wins, as the Mac modding notes say; files without the `.archive` extension
   (`.archive.off`) are not mounted; each archive is mounted once. A bad archive is a fatal game assertion ("Failed to create the archive").
   Archives that the CP2077 Mac Archive Patcher already baked into the official archives must not be mounted a second time (the loose copy
   would win over the baked one): list their file names, one per line, in `mac-mount-skip.txt` next to `ArchiveXL.dylib` (`#` comments
   allowed; template `macos/mac-mount-skip.example.txt`). Each skipped archive is logged once, and the `.xl` file next to it is still found. The
   launcher installs the owner's list (codeman101's setup) with the source ArchiveXL for a session and removes it with the session folder. Boot `r22p`: three skipped,
   only `LizziesBDs.archive` mounted (Mod group with 1 archive), all four `.xl` files found, every probe 1.
2. **The packed `ArchiveXL.archive` bundle is not built** (it needs WolvenKit). The plugin's `Bundle` folder is registered only if it exists.
3. **Hooks and rows are held back until their module is ported.** `macos/MacEnabledRows.inc` lists the ArchiveXL address rows that resolve to
   a real address; every other ArchiveXL row resolves to a trap stub, whose hook is refused and whose direct call returns zero, so a module
   whose signature, layouts or callers are not ported yet cannot damage a session.
4. **`TPPRepresentationComponent_IsAffectedSlot` and `AppearanceChanger_GetSuffixValue` are inlined on Mac** (see the module c notes).

5. **`CMesh::PostLoad` inlines `ShouldPreloadAppearances`.** The Mesh extension's hook on the function therefore misses the PostLoad caller.
   The preload action behind the inlined test (Mac `0xE17504`, a single-caller function; row `CMesh_PreloadAppearances`, Mac-only id
   `0xC0DE1003`) is hooked with the same rule (single-appearance meshes with a `@` template material are not preloaded). Menu boot r22ad: the
   hooks ran (FindAppearance 140, LoadMaterialsAsync 186, preload action 2).
6. **`ResourceLoader::IssueLoadingRequestByPath` inlines `IssueLoadingRequest`** (117 callers against 42 for the request form). The
   resource link extension hooks both (Mac `0x21B71CC` and `0x21B7130`); a hook on the request form alone would miss about three quarters of
   all loads.
7. **`GarmentAssembler_ExtractComponentsJob` frees its closure before it returns.** The Windows row `0x27399C` is the second stage `0xAE6660` on the
   Mac (the fork DB value `0xAE6348` is stage 1, the `ProcessLoadedAppearanceParts` job, which frees its own 0x40 byte block as well, so a `HookAfter` on it would read a
   block that is already gone; R25 comparison with jackmaxwil's port); stage 2 releases the members of its 0x30 byte block and frees the block from the Jobs2Data pool
   itself. The Windows `HookAfter` therefore became a full hook: the handler copies the part template Handle, the context pointer pair (with a
   strong reference of its own) and the 16 flag bytes before the call, calls the original, and runs every patch template's extraction on a
   fresh block from the Jobs2Data pool. Only entity template patches that add `components` use it; hide/show tags and `overrides` do not.
   The hook is attached in `Configure()` only when a loaded patch resource is an `.ent` that modifies components (log line `garment package patch hook not
   needed (no .ent patch targets)` otherwise; `ARCHIVEXL_MAC_GARMENT_JOB=0/1` forces it). Boots r22ah to r22aj: the hook attached and the
   job did not run at the menu, so the clone path is untested.
8. **The memory vault serves every game pool.** ArchiveXL allocates from many pools (Engine, RefCount, Default, Mesh, EntityResource, ...);
   a missing pool is a fault at `Allocator<Pool>::Free`. `macos/MacPools.inc` (written by `tools/mac/gen-pools.py` from the game
   executable) lists all 650-odd exported pools and the vault resolves a pool's five functions with `dlsym` at its first use.
9. **`ResourceLoader_OnUpdate` and the world widget limit are mapped and working.** The hot reload trigger of `ExtensionService::Configure` is the
   loader update `0x21B5548` (boot r22an ran a full reload through it). `InkWorldLayer_UpdateComponents` is inlined into a parallel-for job body
   (`0x491DDB0`, row read as code); the limit `cmp x22,#9; b.hi` at `+0x130` is nopped at boot (r22ao). The row `WorldWidgetLimitPatch` writes the code
   page with `vm_protect(VM_PROT_COPY)`, which the hardened game allows.
10. **Declared but never used rows** (`IPlacedComponent_SetTransform`, `MorphTargetManager_ApplyMorphTarget`, the `Entity_*` rows other than
    `Entity_Reassemble`, `AppearanceChangeSystem_ChangeAppearance1/2`) are not referenced by any ArchiveXL code path; they stay held back.
11. **The ink spawner fallback hooks run only without Codeware 1.14.0 or newer.** With Codeware (always on the Mac) the extension installs no
    hooks; the Mac signatures of the fallback (Handle results through x8, one extra bool on the async functions) are ported but untested.

12. **A Mac target can carry only one hook, so ArchiveXL and Codeware share it.** The Frida based hooking driver puts one detour on a target and the
    second plugin that wants the same target is chained behind the first (`<owner>_mac_hook_chain_attach`, found with `dlsym`; each plugin keeps
    its own typed trampoline, so the Raw argument lists only have to agree in registers). Targets that both plugins hook in the current setup
    (boot r22ag): `CBaseEngine::LoadGatheredResources` `0x3D9EFC8` (owner ArchiveXL), `CBaseEngine::InitEngine` `0x3D9E494`,
    `Localization::LoadOnScreens` `0x2F67DB4` and `Entity::Reassemble` `0xC95744` (owner Codeware). On Windows RED4ext supports any number of
    hooks per target; this is a Mac hooking driver gap, documented in `MacHookChain.cpp`.

13. **Archive order is the game's byte order; `modlist.txt` is not read.** `App::ArchiveService` sorts `archive/Mac/mod/*.archive` by byte value (case
    sensitive). That is the game's own rule, not an approximation: the Windows engine collects a group with the `*.archive` finder (`0x7bb008`) and sorts the
    names with an insertion sort (`0x7bb0bc`, the introsort above 32 entries) whose comparator `0xbe0efc` compares two CStrings byte by byte, unsigned, with no
    case folding. A case-insensitive order (the NTFS listing order, used by jackmaxwil's port) would put `zebra.archive` before `_x.archive` (the upper-cased 'Z' is below '_'); the game puts
    it after. The same engine function (`0xe60b5c`) also looks for `modlist.txt` in the mod folder and takes an explicit order from it when it exists. ArchiveXL does
    not read `modlist.txt` on the Mac, so a mod that ships one is ordered by name. (R25: the comparator was read in the Windows binary; the Mac depot builds its
    three groups from fixed name prefixes (`basegame_`, `audio_`, `lang_`) and has no mod group, so there is no Mac rule to match.)

## Upstream (Windows) findings

Candidates for an upstream issue. The Windows values in this fork are NOT changed; whether to file anything is decided at publish time.

1. **The Windows reference table pairs `MappinSystem_GetMappinData` and `MappinSystem_GetPoiData` with crossed addresses.** The Mac
   functions read `this+0x58` (the cooked mappin resource) and `this+0x68` (the POI resource), as the offsets in `Red/MappinSystem.hpp` say;
   the Windows address 0x9067A0 (named GetMappinData) reads +0x68 and 0x69BA6C (named GetPoiData) reads +0x58.
