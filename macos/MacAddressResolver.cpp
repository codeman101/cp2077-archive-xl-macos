#include "MacAddressResolver.hpp"

#include <RED4ext/Relocation.hpp>

#include "Red/Addresses/Library.hpp"

#include <cstdio>
#include <mutex>
#include <unordered_map>
#include <unordered_set>

namespace
{
// RVAs from the Mac image (Steam 2.3.1) for the hashes of ArchiveXL's src/Red/Addresses/Library.hpp. Every row was first read from the
// RED4ext.SDK-macos address database of the jackmaxwil port (verified there) or from Codeware's rows, and is rated here against the
// Windows twin by tools/addr-match/check-rows.py. Rows the database does not verify are left out and resolve to a trap.
// ArchiveXL's own rows. Each hash is held back (resolves like an unmapped function: a trap stub) until its module is ported to the Mac ABI
// and listed in MacEnabledRows.inc: a row that exists is not a function whose Windows signature, struct layouts and callers are ported.
const std::unordered_map<uint32_t, uintptr_t>& AxlRows()
{
    static const std::unordered_map<uint32_t, uintptr_t> rows = {
        {2515346652u, 0x3D515D0}, // AIWorkspotManager_RegisterSpots. Windows 0x83bf4c. fork DB, verified there.
        {2855474741u, 0xB97524}, // AnimatedComponent_InitializeAnimations. Windows 0x46b7b4. fork DB, verified there.
        {735526026u, 0xD43218}, // AppearanceChangeSystem_ChangeAppearance1. Windows 0x85084c. fork DB, verified there.
        {386815609u, 0xD45FD0}, // AppearanceChangeSystem_ChangeAppearance2. Windows 0x8a5b34. fork DB, verified there.
        {3243419919u, 0x3710560}, // AppearanceChanger_ComputePlayerGarment. Windows 0xb58674. fork DB, verified there.
        {63057648u, 0x370D924}, // AppearanceChanger_GetSuffixes. Windows 0x858b20. fork DB, verified there.
        {3169139695u, 0xCD1F50}, // AppearanceChanger_RegisterPart. Windows 0xa9fcbc. fork DB, verified there.
        {2770550105u, 0x3732C68}, // AppearanceChanger_SelectAppearanceName. Windows 0x8de198. fork DB, verified there.
        {39206067u, 0xACCA30}, // AppearanceDefinition_ExtractPartComponents. Windows 0x5067f4. fork DB, verified there.
        {1186798404u, 0x3CD7234}, // AppearanceNameVisualTagsPreset_GetVisualTags. Windows 0x3f1334. fork DB, verified there.
        {549398675u, 0xAD7048}, // AppearanceResource_FindAppearanceDefinition. Windows 0x8aa5e8. fork DB, verified there.
        {3141736993u, 0xAD88A8}, // AppearanceResource_PostLoad. Windows 0x105d42c. fork DB, verified there.
                {3224838039u, 0x3508DF8}, // AttachmentSlots_InitializeSlots. Windows 0x3f0874. R22 code twin: Windows 0x3F0874 is the unlocked body (its only caller 0x81F788 takes the lock at this+0x100); Mac 0x3508DF8 is its exact twin and 0x3508DB0 is the locking wrapper (single caller 0x3894A6C). Hooked at the body so the handler runs under the slot lock as on Windows.
        {4231927464u, 0x35104E4}, // AttachmentSlots_IsSlotEmpty. Windows 0x6afce0. fork DB, verified there.
        {1283201918u, 0x3510644}, // AttachmentSlots_IsSlotSpawning. Windows 0xffc6ac. fork DB, verified there.
        {1299073554u, 0x22643C0}, // BufferReader_MakeType0. Windows 0x23697c. fork DB, verified there.
        {3578095989u, 0x226442C}, // BufferReader_MakeType1. Windows 0x2094c08. fork DB, verified there.
        {3273923080u, 0x3D9E494}, // CBaseEngine_InitEngine. Windows 0x7cb080. fork DB, verified there.
        {3729789488u, 0x3D9EFC8}, // CBaseEngine_LoadGatheredResources. Windows 0x303460. fork DB, verified there.
        {555290091u, 0xE16E68}, // CMesh_AddStubAppearance. Windows 0x223b3a8. fork DB, verified there.
        {3007126651u, 0xE189F4}, // CMesh_FindAppearance. Windows 0x3ca158. fork DB, verified there.
        {773462733u, 0xE199D8}, // CMesh_GetAppearance. Windows 0x3ca128. fork DB, verified there.
        {701648326u, 0xE18C28}, // CMesh_LoadMaterialsAsync. Windows 0x253c5c. fork DB, verified there.
        {2272530537u, 0xE16B28}, // CMesh_PostLoad. Windows 0x224844. fork DB, verified there.
        {503977943u, 0xE173FC}, // CMesh_ShouldPreloadAppearances. Windows 0x22583c. fork DB, verified there.
        {3015323410u, 0x24436D0}, // CharacterCustomizationFeetController_CheckState. Windows 0x388134. fork DB, verified there.
        {1782982010u, 0x2440418}, // CharacterCustomizationGenitalsController_OnAttach. Windows 0x18b2c14. fork DB, verified there.
        {2652844338u, 0x2442D8C}, // CharacterCustomizationHairstyleController_CheckState. Windows 0x8579d4. fork DB, verified there.
        {2249860539u, 0x24415C0}, // CharacterCustomizationHairstyleController_OnDetach. Windows 0x388690. fork DB, verified there.
        {110771572u, 0x2451F2C}, // CharacterCustomizationState_GetArmsAppearances1. Windows 0x1192f2c. fork DB, verified there.
        {4059182261u, 0x2452144}, // CharacterCustomizationState_GetArmsAppearances2. Windows 0x1192f2c. fork DB, verified there.
        {4014159024u, 0x2451AFC}, // CharacterCustomizationState_GetBodyAppearances1. Windows 0xeede78. fork DB, verified there.
        {56573295u, 0x2451D14}, // CharacterCustomizationState_GetBodyAppearances2. Windows 0xb85240. fork DB, verified there.
        {4051123539u, 0x2451620}, // CharacterCustomizationState_GetHeadAppearances1. Windows 0x38503c. fork DB, verified there.
        {3766105236u, 0x24518E4}, // CharacterCustomizationState_GetHeadAppearances2. Windows 0x38503c. fork DB, verified there.
        {4275058446u, 0x2459710}, // CharacterCustomizationSystem_GetResource. Windows 0xeed744. fork DB, verified there.
        {2341291776u, 0x24593DC}, // CharacterCustomizationSystem_Initialize. Windows 0x27f8a0c. fork DB, verified there.
        {1092645213u, 0x2464ED4}, // CharacterCustomizationSystem_InitializeAppOption. Windows 0x27f5010. fork DB, verified there.
        {3776734053u, 0x24652D8}, // CharacterCustomizationSystem_InitializeMorphOption. Windows 0x27f5400. fork DB, verified there.
        {2347184335u, 0x2440814}, // CharacterCustomizationGenitalsController_CheckState. Windows 0x8586b4. R22: body twin read from both sides (virtual 0x2A0 / Mac 0x2A8 first, then the owner's component at +0x50 -> +0xC0, the vtable +0x430 / Mac +0x438 query, the 0x90 byte element loop, the controller flag at +0xD0 / Mac +0xC8) and the Mac state refresh virtual 0x2440754 calls it as the Windows 0x3861dc calls 0x8586b4. The fork DB entry 0x2deac58 was unverified.
        {2229107513u, 0x24661A0}, // CharacterCustomizationSystem_InitializeSwitcherOption. Windows 0x27f5cb0. fork DB, verified there.
        {402202441u, 0x2459F78}, // CharacterCustomizationSystem_Uninitialize. Windows 0x27f81bc. fork DB, verified there.
        {489494088u, 0xCA41E0}, // EntityBuilder_ExtractComponentsJob. Windows 0x1cd4a8. Codeware/SDK row.
        {437791594u, 0xCA08A0}, // EntityBuilder_ScheduleExtractComponentsJob. Windows 0x8ac678. fork DB, verified there.
        {2509382878u, 0x14084B0}, // EntitySpawner_SpawnFromTemplate. Windows 0x661538. fork DB, verified there.
        {36838056u, 0xCB12BC}, // EntityTemplate_FindAppearance. Windows 0x8ac190. fork DB, verified there.
        {2741376473u, 0xCB2A6C}, // EntityTemplate_OnLoad. Windows 0x26f3e8. fork DB, verified there.
        {2182550867u, 0xC97E28}, // Entity_Assemble. Windows 0x8ab708. Codeware/SDK row.
        {4248638169u, 0xC99368}, // Entity_Attach. Windows 0x1cdbb0. Codeware/SDK row.
        {2263681375u, 0xC996C0}, // Entity_Detach. Windows 0x1b3a1c. Codeware/SDK row.
        {2515274237u, 0xC999E0}, // Entity_Dispose. Windows 0x8b5540. Codeware/SDK row.
        {3490519617u, 0xC985EC}, // Entity_Initialize. Windows 0x8b0190. Codeware/SDK row.
        {1560690857u, 0xC95744}, // Entity_Reassemble. Windows 0xc27fd8. fork DB, verified there.
        {2296260874u, 0xC95E24}, // Entity_RequestComponents. Windows 0x8b116c. Codeware/SDK row.
        {3596356635u, 0xC992C4}, // Entity_Uninitialize. Windows 0x402f74. Codeware/SDK row.
        {1886854234u, 0xCC0710}, // FactoryIndex_LoadFactoryAsync. Windows 0x94d51c. fork DB, verified there.
        {3040549301u, 0xCC0BEC}, // FactoryIndex_ResolveResource. Windows 0x2046bf4. fork DB, verified there.
        {2923109755u, 0x1704194}, // GameApplication_InitResourceDepot. Windows 0x9f45c8. fork DB, verified there.
        {3128897273u, 0x36F82F4}, // GarmentAssemblerState_AddCustomItem. Windows 0xb56ac0. fork DB, verified there.
        {956641309u, 0x36F89A4}, // GarmentAssemblerState_ChangeCustomItem. Windows 0xb56a1c. R22: exact twin by the read-only call structure (four helper calls ANDed: remove by hash, the 0x9810/0xb5807c pair, the 0x8d24/0x5e9ffc one with a zero third argument, and the overrides variant with request+0x10; request hash at +0x20, index at +0x28 on both sides). The fork DB entry 0x36fe44c is a log message function.
        {3740082313u, 0x36F887C}, // GarmentAssemblerState_ChangeItem. Windows 0xb57fd8. R22: exact twin (same four calls ANDed, request hash at +0x10, index at +0x18). The fork DB entry 0x36fe44c is a log message function.
        {2294423397u, 0x36F8208}, // GarmentAssemblerState_AddItem. Windows 0x5e9e00. fork DB, verified there.
        {809178766u, 0xAE6660}, // GarmentAssembler_ExtractComponentsJob. Windows 0x27399c. R22 code twin: the Windows body is the second stage 0xAE6660 (the fork DB value 0xAE6348 is stage 1). Both Mac stages free their closure at the end, so the HookAfter handler cannot run; the row stays held back (NOT_YET.md).
        {2594581880u, 0x36F8030}, // GarmentAssembler_FindState. Windows 0x5eb300. fork DB, verified there.
        {709304039u, 0x371FE6C}, // GarmentAssembler_OnGameDetach. Windows 0x70437c. R22 code twin: void(this, a2), called only from the entity appearance system's detach virtual (Windows 0x557D98 / Mac 0x3712FF8). The fork DB value 0x36F6F4C is the callee that clears the per-entity states.
        {29053464u, 0xAE4004}, // GarmentAssembler_ProcessGarment. Windows 0xb57c60. fork DB, verified there.
        {1567972572u, 0xAE3CC0}, // GarmentAssembler_ProcessMorphedMesh. Windows 0x79696c. fork DB, verified there.
        {1663588463u, 0xAE3840}, // GarmentAssembler_ProcessSkinnedMesh. Windows 0x7974b0. fork DB, verified there.
        {1863723270u, 0x36F8744}, // GarmentAssembler_RemoveItem. Windows 0xb575bc. fork DB, verified there.
        {1828854026u, 0xC417F0}, // IPlacedComponent_SetTransform. Windows 0x574fc8. Codeware/SDK row.
        {2698985195u, 0x49799B8}, // InkSpawner_FinishAsyncSpawn. Windows 0xfa63f0. fork DB, verified there.
        {1396063719u, 0x4965B38}, // InkWidgetLibrary_AsyncSpawnFromExternal. Windows 0x9fb2a8. fork DB, verified there.
        {118698863u, 0x4965980}, // InkWidgetLibrary_AsyncSpawnFromLocal. Windows 0x57f26c. fork DB, verified there.
        {506278179u, 0x4965EC0}, // InkWidgetLibrary_SpawnFromExternal. Windows 0x9fb618. fork DB, verified there.
        {1158555307u, 0x4965DE0}, // InkWidgetLibrary_SpawnFromLocal. Windows 0x57f9c0. fork DB, verified there.
        {3392610574u, 0x36FCE14}, // ItemFactoryAppearanceChangeRequest_LoadAppearance. Windows 0x12e1a38. fork DB, verified there.
        {1291460507u, 0x36FCA88}, // ItemFactoryAppearanceChangeRequest_LoadTemplate. Windows 0x1387648. fork DB, verified there.
        {3659799256u, 0x37034A4}, // ItemFactoryRequest_LoadAppearance. Windows 0x5ebbf0. fork DB, verified there.
        {1576079097u, 0x9D479C}, // JobHandle_Wait. Windows 0x2ac294. fork DB, verified there.
        {1964512489u, 0x1EAE820}, // JournalManager_LoadJournal. Windows 0x9522c0. fork DB, verified there.
        {3274382593u, 0x1EB17C0}, // JournalManager_TrackQuest. Windows 0x5944fc. fork DB, verified there.
                {1945256833u, 0x1E75A24}, // JournalRootFolderEntry_Initialize. Windows 0x950620. The virtual (this, a2, a3, JobQueue) that calls the loader half 0x1E7528C and tail-branches to 0x1E75660, as the Windows virtual 0x950620 calls 0x9507fc then jumps to 0x950660. R22: the fork DB row 0x1E7528C is only the first half.
        {837162664u, 0x1EA3410}, // JournalTree_ProcessJournalIndex. Windows 0x94ffcc. fork DB, verified there.
        {1488657506u, 0x1AF9CCC}, // Localization_LoadLipsyncs. Windows 0x112cb18. fork DB, verified there.
        {3550098299u, 0x2F67DB4}, // Localization_LoadOnScreens. Windows 0x261018. fork DB, verified there.
        {772484645u, 0x2F67BE0}, // Localization_LoadSubtitles. Windows 0x262090. fork DB, verified there.
        {4223669659u, 0x1405334}, // Localization_LoadVoiceOvers. Windows 0x263124. fork DB, verified there.
        {240386859u, 0x31E18}, // Main. Windows 0x18d4dc0. Codeware/SDK row.
        {3299551353u, 0x42A3E68}, // MappinSystem_GetMappinData. Windows 0x9067a0 (reads this+0x68). R22: the fork DB row 0x42A3E38 is the crossed twin; the Mac wrapper that reads this+0x68 is 0x42A3E68. Not hooked (16 byte wrapper), see FindPoiData.
        {620961393u, 0x42A3E38}, // MappinSystem_GetPoiData. Windows 0x69ba6c (reads this+0x58). R22: crossed in the fork DB; the Mac wrapper that reads this+0x58 is 0x42A3E38. Not hooked (16 byte wrapper), see FindMappinData.
        {3235778561u, 0x429EED0}, // MappinSystem_FindMappinData (Mac-only id). R22: the finder (resource +0x40 array, count +0x4C, stride 0x20, u32 key) behind the 0x42A3E38 wrapper; 72 byte leaf, also called directly by the inlined position lookup 0x42B2748.
        {3235778562u, 0x42BE574}, // MappinSystem_FindPoiData (Mac-only id). R22: the same finder shape for the POI resource, behind the 0x42A3E68 wrapper.
        {3235778563u, 0xE17504}, // CMesh_PreloadAppearances (Mac-only id). R22: the action CMesh::PostLoad takes after its inlined ShouldPreloadAppearances test; (CMesh*, w1 = 0, request*), void, single caller at 0xE16D00. Windows twin 0xAF3CC4 (caller 0x224907).
        {3235778564u, 0x36F87E4}, // GarmentAssembler_ChangeItem (Mac-only id). R22: aggregator level wrapper bool(aggregator, WeakHandle<Entity>&, GarmentItemChangeRequest&), called once from 0x36FDDD4 (twin of the Windows request Apply 0xB57D10). The state level body 0x36F887C has no callers.
        {3235778565u, 0x36F890C}, // GarmentAssembler_ChangeCustomItem (Mac-only id). R22: the same for the custom request, called once from 0x36FDDD4.
        {3235778566u, 0x3CD7118}, // AppearanceNameVisualTagsPreset_GetVisualTagsByRecord (Mac-only id). R22: twin of the Windows wrapper 0x3F12F0 (record based entry), the map find is inlined and it does not call 0x3CD7234. R26: the TweakDBID field it reads at +0xE4 holds the game's cached flat offset (see GarmentExtension ResolveRecordFlatPath); the Windows wrapper reads +0xE8.
        {3235778567u, 0x35A6654}, // TPPRepresentationComponent_UnregisterAffectedItem (Mac-only id). R22: void(TPPRepresentationComponent*, TweakDBID aItemID), the cold call of OnItemUnequipped for an affected slot; Windows twin 0x25050CC.
        {787099588u, 0x2456C9C}, // CharacterCustomizationSystem_EnsureState. Windows 0xeed694 (cold half 0x1fc6c0a). R22 code twin: (System*, Handle<State>*), one of the three sites that inline GetResource on the Mac (the GetResource row 0x2459710 has no callers).
        {140387944u, 0x42A71A4}, // MappinSystem_OnStreamingWorldLoaded. Windows 0x1636a4. fork DB, verified there.
        {1419388740u, 0xE1F7B4}, // MeshAppearance_LoadMaterialSetupAsync. Windows 0x21a670. fork DB, verified there.
        {1609519068u, 0xE27A4C}, // MeshMaterialBuffer_LoadMaterialAsync. Windows 0x2781a0. fork DB, verified there.
        {633871992u, 0xC6C0C4}, // MorphTargetManager_ApplyMorphTarget. Windows 0x1607fc. Codeware/SDK row.
        {1523127443u, 0xE467BC}, // MorphTargetMesh_PostLoad. Windows 0xc27adc. fork DB, verified there.
        {546121377u, 0x3FEBC30}, // PersistencySystem_SetPersistentStateData. Windows 0x2494e0. fork DB, verified there.
        {790570700u, 0x2ED6090}, // QuestLoader_ProcessPhaseResource. Windows 0x9461ac. fork DB, verified there.
        {797843833u, 0x2D00870}, // QuestRootInstance_Start. Windows 0x270026c (the virtual with the post-create helper). R22: the fork DB row 0x2D01878 is the twin of the other Windows function 0x11FC720 (not virtual, no helper); the virtual is 0x2D00870 (questRootInstance vtable slot 0xE8), with the post-create helper 0x2CFACF4.
        {2048921710u, 0x2EF8C58}, // QuestsSystem_OnGameRestored. Windows 0x495fd0. fork DB, verified there.
        {43194193u, 0x3ED9E9C}, // ResourceDepot_CheckResource. Windows 0x26475c. fork DB, verified there.
        {2885423437u, 0x3ED96B0}, // ResourceDepot_InitializeArchives. Windows 0xe60e24. fork DB, verified there.
        {2517385486u, 0x3EDA488}, // ResourceDepot_LoadArchives. Windows 0x7bba48. fork DB, verified there.
        {2450934495u, 0x3ED9B94}, // ResourceDepot_RequestResource. Windows 0x1cfa8c. fork DB, verified there.
        {2365013187u, 0x21B71CC}, // ResourceLoader_RequestResource. Windows 0x1de50c. fork DB, verified there.
        {3998356057u, 0x21C90A4}, // ResourcePath_Create. Windows 0x1df644. fork DB, verified there.
        {2901686778u, 0x225921C}, // ResourceSerializer_Deserialize. Windows 0x148b70. fork DB, verified there.
        {2577814646u, 0x2255B4C}, // ResourceSerializer_Load. Windows 0x2357e8. fork DB, verified there.
        {1185093671u, 0x22575C0}, // ResourceSerializer_OnDependenciesReady. Windows 0x24486c. fork DB, verified there.
        {3972601611u, 0x3426204}, // StreamingSector_PostLoad. Windows 0x26eeb4. fork DB, verified there.
        {410718963u, 0x342AD40}, // StreamingWorld_Serialize. Windows 0x113a890. fork DB, verified there.
        {4129169021u, 0x35A0604}, // TPPRepresentationComponent_OnAttach. Windows 0xac58c0. fork DB, verified there.
        {4010810747u, 0x35A6174}, // TPPRepresentationComponent_OnItemEquipped. Windows 0x9fe5f4. fork DB, verified there.
        {1933319146u, 0x35A56DC}, // TPPRepresentationComponent_OnItemUnequipped. Windows 0x9fe1dc. fork DB, verified there.
        {3037343626u, 0x35A0E10}, // TPPRepresentationComponent_RegisterAffectedItem. Windows 0x9fdce8. fork DB, verified there.
        {3602585178u, 0x2B75570}, // TweakDB_Load. Windows 0x50eb1c. fork DB, verified there.
        // Unmapped (trap stubs):
        // AISpotPersistentDataArray_Reserve (3842971216). Windows 0x83bf1c. unmapped: stays a trap (fork DB entry 0x3040dec unverified).
        // AppearanceChanger_GetBaseMeshOffset (4219677283). Windows 0x2716f8. unmapped: stays a trap (fork DB entry 0xad043c unverified).
        // AppearanceChanger_GetSuffixValue (1003499294). Windows 0x8581fc. unmapped: stays a trap (fork DB entry 0x370e9ac unverified).
        // CNamePool_GetStringView (2878476560). Windows 0x145548. unmapped: stays a trap (fork DB entry 0x1e3f8f4 unverified).
        // CNamePool_RegisterName (627643257). Windows 0x97cdc0. unmapped: stays a trap (fork DB entry 0x1e3ed04 unverified).
        // CharacterCustomizationHelper_GetHairColor (3414699684). Windows 0xeed518. unmapped: stays a trap (fork DB entry 0x1441630 unverified).
        // CharacterCustomizationState_FinalizePart (716262863). Windows 0x27f3610. unmapped: stays a trap (fork DB entry 0x3d783b8 unverified).
        // CharacterCustomizationSystem_EnsureState (787099588). Windows 0xeed694. unmapped: stays a trap (fork DB entry 0x2f2d360 unverified).
        // CharacterCustomizationSystem_InitializeOptionsFromState (1168449474). Windows 0x27f5600. unmapped: stays a trap (fork DB entry 0x453c2d4 unverified).
        // ImpostorComponent_OnAttach (4002093843). Windows 0x2570a70. unmapped: stays a trap (fork DB entry 0x37dcb7c unverified).
        {1693194103u, 0x491DDB0}, // InkWorldLayer_UpdateComponents. Windows 0x29eebc. R22 code twin: no standalone function on the Mac; the lambda is inlined into the parallel-for job body 0x491DDB0 (Windows job body 0xC51EEC). The limit is `cmp x22,#9; b.hi` at +0x12C/+0x130. The row is read as code (WorldWidgetLimitPatch), never called. The fork DB entry 0x3da0574 was unverified.
        {1303056161u, 0x21B5548}, // ResourceLoader_OnUpdate. Windows 0x270acc. R22 code twin: void(ResourceLoader*), the loader's per-frame token cache maintenance, one caller (the tail call wrapper 0x21B99A8, called once per frame from 0x3D99938 like Windows 0x29329C from 0x291DF0). The fork DB entry 0x2d9766c was unverified.
        // ResourceSerializer_OnResourceReady (1147149338). Windows 0x235784. unmapped: stays a trap (fork DB entry 0x20ab510 unverified).
        // ResourceSerializer_PostLoad (1350901460). Windows 0x243138. unmapped: stays a trap (fork DB entry 0x21ac670 unverified).
        // TPPRepresentationComponent_IsAffectedSlot (678894266). Windows 0x9fe870. unmapped: stays a trap (fork DB entry 0x23d9df4 unverified).
    };
    return rows;
}

// Rows of SDK hashes shared with Codeware (game facts, copied from its MacSdkRows.inc; the SDK overlay injects the same file). Never held back.
const std::unordered_map<uint32_t, uintptr_t>& SdkRows()
{
    static const std::unordered_map<uint32_t, uintptr_t> rows = {
#include "MacSdkRows.inc"
    };
    return rows;
}

// Hashes of AxlRows that are enabled. The list grows module by module (R22: a, b, c, d).
const std::unordered_set<uint32_t>& EnabledRows()
{
    static const std::unordered_set<uint32_t> enabled = {
#include "MacEnabledRows.inc"
    };
    return enabled;
}
}

uintptr_t Support::MacAddressResolver::ResolveAddress(uint32_t aAddressID)
{
    const auto& sdkRows = SdkRows();
    const auto sit = sdkRows.find(aAddressID);
    if (sit != sdkRows.end())
    {
        return Red::MacGameBase() + sit->second;
    }

    const auto& rows = AxlRows();
    const auto it = rows.find(aAddressID);
    if (it != rows.end())
    {
        if (EnabledRows().count(aAddressID))
        {
            return Red::MacGameBase() + it->second;
        }
        // Held back: falls through to the trap table (a hook on it is refused, a direct call logs "not on Mac yet" and returns zero).
    }

    // The SDK resolver holds the shared Mac rows (CRTTISystem_Get, CClass, function constructors) and, in this build, falls back to
    // the trap table (MacTrap.cpp) for a hash that has no row anywhere: the returned address is a stub that logs
    // "[axl] not on Mac yet: <name>" on its first call and returns zero. Only an id that is not in the table at all comes back as 0.
    return RED4ext::UniversalRelocBase::Resolve(aAddressID);
}
