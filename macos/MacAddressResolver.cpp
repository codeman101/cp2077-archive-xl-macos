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
        {2515346652u, 0x267F100}, // AIWorkspotManager_RegisterSpots. Windows 0x83bf4c. fork DB, verified there.
        {2855474741u, 0x18A5174}, // AnimatedComponent_InitializeAnimations. Windows 0x46b7b4. fork DB, verified there.
        {735526026u, 0x1A59E30}, // AppearanceChangeSystem_ChangeAppearance1. Windows 0x85084c. fork DB, verified there.
        {386815609u, 0x1A5C6E0}, // AppearanceChangeSystem_ChangeAppearance2. Windows 0x8a5b34. fork DB, verified there.
        {3243419919u, 0x2020a80}, // AppearanceChanger_ComputePlayerGarment. Windows 0xb58674. fork DB, verified there.
        {63057648u, 0x201DCC8}, // AppearanceChanger_GetSuffixes. Windows 0x858b20. fork DB, verified there.
        {3169139695u, 0x19E7BC0}, // AppearanceChanger_RegisterPart. Windows 0xa9fcbc. fork DB, verified there.
        {2770550105u, 0x2043B04}, // AppearanceChanger_SelectAppearanceName. Windows 0x8de198. fork DB, verified there.
        {39206067u, 0x17D3158}, // AppearanceDefinition_ExtractPartComponents. Windows 0x5067f4. fork DB, verified there.
        {1186798404u, 0x2605214}, // AppearanceNameVisualTagsPreset_GetVisualTags. Windows 0x3f1334. fork DB, verified there.
        {549398675u, 0x17DD6C0}, // AppearanceResource_FindAppearanceDefinition. Windows 0x8aa5e8. fork DB, verified there.
        {3141736993u, 0x17DF10C}, // AppearanceResource_PostLoad. Windows 0x105d42c. fork DB, verified there.
                {3224838039u, 0x1E05EEC}, // AttachmentSlots_InitializeSlots. Windows 0x3f0874. R22 code twin: Windows 0x3F0874 is the unlocked body (its only caller 0x81F788 takes the lock at this+0x100); Mac 0x3508DF8 is its exact twin and 0x3508DB0 is the locking wrapper (single caller 0x3894A6C). Hooked at the body so the handler runs under the slot lock as on Windows.
        {4231927464u, 0x1E0D97C}, // AttachmentSlots_IsSlotEmpty. Windows 0x6afce0. fork DB, verified there.
        {1283201918u, 0x1e0dadc}, // AttachmentSlots_IsSlotSpawning. Windows 0xffc6ac. fork DB, verified there.
        {1299073554u, 0x121743C}, // BufferReader_MakeType0. Windows 0x23697c. fork DB, verified there.
        {3578095989u, 0x12174AC}, // BufferReader_MakeType1. Windows 0x2094c08. fork DB, verified there.
        {3273923080u, 0x4D7EFB8}, // CBaseEngine_InitEngine. Windows 0x7cb080. fork DB, verified there.
        {3729789488u, 0x4D7FBA8}, // CBaseEngine_LoadGatheredResources. Windows 0x303460. fork DB, verified there.
        {555290091u, 0x519A844}, // CMesh_AddStubAppearance. Windows 0x223b3a8. fork DB, verified there.
        {3007126651u, 0x519C4B0}, // CMesh_FindAppearance. Windows 0x3ca158. fork DB, verified there.
        {773462733u, 0x519E118}, // CMesh_GetAppearance. Windows 0x3ca128. fork DB, verified there.
        {701648326u, 0x519C6DC}, // CMesh_LoadMaterialsAsync. Windows 0x253c5c. fork DB, verified there.
        {479081020u, 0x519C6DC}, // CMesh_LoadMaterialsAsync: the 2.5 hash of the same function (psiberx/cp2077-archive-xl#31 re-derived it on Windows; this row is the 2.5 Mac address of the old row)
        {2272530537u, 0x519A58C}, // CMesh_PostLoad. Windows 0x224844. fork DB, verified there.
        {503977943u, 0x519ADA4}, // CMesh_ShouldPreloadAppearances. Windows 0x22583c. fork DB, verified there.
        {3015323410u, 0x4BB13FC}, // CharacterCustomizationFeetController_CheckState. Windows 0x388134. fork DB, verified there.
        {1782982010u, 0x4bae148}, // CharacterCustomizationGenitalsController_OnAttach. Windows 0x18b2c14. fork DB, verified there.
        {2652844338u, 0x4BB0AC0}, // CharacterCustomizationHairstyleController_CheckState. Windows 0x8579d4. fork DB, verified there.
        {2249860539u, 0x4BAF2C0}, // CharacterCustomizationHairstyleController_OnDetach. Windows 0x388690. fork DB, verified there.
        {110771572u, 0x4BBFC78}, // CharacterCustomizationState_GetArmsAppearances1. Windows 0x1192f2c. fork DB, verified there.
        {4059182261u, 0x4bbff38}, // CharacterCustomizationState_GetArmsAppearances2. Windows 0x1192f2c. fork DB, verified there.
        {4014159024u, 0x4BBF6F8}, // CharacterCustomizationState_GetBodyAppearances1. Windows 0xeede78. fork DB, verified there.
        {56573295u, 0x4BBF9B8}, // CharacterCustomizationState_GetBodyAppearances2. Windows 0xb85240. fork DB, verified there.
        {4051123539u, 0x4bbf004}, // CharacterCustomizationState_GetHeadAppearances1. Windows 0x38503c. fork DB, verified there.
        {3766105236u, 0x4BBF438}, // CharacterCustomizationState_GetHeadAppearances2. Windows 0x38503c. fork DB, verified there.
        {4275058446u, 0x4bc7b24}, // CharacterCustomizationSystem_GetResource. Windows 0xeed744. fork DB, verified there.
        {2341291776u, 0x4BC779C}, // CharacterCustomizationSystem_Initialize. Windows 0x27f8a0c. fork DB, verified there.
        {2343126786u, 0x4BC779C}, // CharacterCustomizationSystem_Initialize: the 2.5 hash of the same function (psiberx/cp2077-archive-xl#31 re-derived it on Windows; this row is the 2.5 Mac address of the old row)
        {1092645213u, 0x4BD3F5C}, // CharacterCustomizationSystem_InitializeAppOption. Windows 0x27f5010. fork DB, verified there.
        {3776734053u, 0x4BD448C}, // CharacterCustomizationSystem_InitializeMorphOption. Windows 0x27f5400. fork DB, verified there.
        {2347184335u, 0x4bae524}, // CharacterCustomizationGenitalsController_CheckState. Windows 0x8586b4. R22: body twin read from both sides (virtual 0x2A0 / Mac 0x2A8 first, then the owner's component at +0x50 -> +0xC0, the vtable +0x430 / Mac +0x438 query, the 0x90 byte element loop, the controller flag at +0xD0 / Mac +0xC8) and the Mac state refresh virtual 0x2440754 calls it as the Windows 0x3861dc calls 0x8586b4. The fork DB entry 0x2deac58 was unverified.
        {2229107513u, 0x4BD53AC}, // CharacterCustomizationSystem_InitializeSwitcherOption. Windows 0x27f5cb0. fork DB, verified there.
        {402202441u, 0x4BC83A8}, // CharacterCustomizationSystem_Uninitialize. Windows 0x27f81bc. fork DB, verified there.
        {489494088u, 0x19B74B0}, // EntityBuilder_ExtractComponentsJob. Windows 0x1cd4a8. Codeware/SDK row.
        {437791594u, 0x19B3E70}, // EntityBuilder_ScheduleExtractComponentsJob. Windows 0x8ac678. fork DB, verified there.
        {2509382878u, 0x46E03B8}, // EntitySpawner_SpawnFromTemplate. Windows 0x661538. fork DB, verified there.
        {36838056u, 0x19C4808}, // EntityTemplate_FindAppearance. Windows 0x8ac190. fork DB, verified there.
        {2741376473u, 0x19C6380}, // EntityTemplate_OnLoad. Windows 0x26f3e8. fork DB, verified there.
        {2182550867u, 0x19AA048}, // Entity_Assemble. Windows 0x8ab708. Codeware/SDK row.
        {4248638169u, 0x529EC50}, // Entity_Attach. Windows 0x1cdbb0. Codeware/SDK row.
        {2263681375u, 0x19AB4DC}, // Entity_Detach. Windows 0x1b3a1c. Codeware/SDK row.
        {2515274237u, 0x19AB7F8}, // Entity_Dispose. Windows 0x8b5540. Codeware/SDK row.
        {3490519617u, 0x19AA7D8}, // Entity_Initialize. Windows 0x8b0190. Codeware/SDK row.
        {1560690857u, 0x19A7A30}, // Entity_Reassemble. Windows 0xc27fd8. fork DB, verified there.
        {2296260874u, 0x19A8040}, // Entity_RequestComponents. Windows 0x8b116c. Codeware/SDK row.
        {3596356635u, 0x19AB2D4}, // Entity_Uninitialize. Windows 0x402f74. Codeware/SDK row.
        {1886854234u, 0x19D50C8}, // FactoryIndex_LoadFactoryAsync. Windows 0x94d51c. fork DB, verified there.
        {1931943018u, 0x19D50C8}, // FactoryIndex_LoadFactoryAsync: the 2.5 hash of the same function (psiberx/cp2077-archive-xl#31 re-derived it on Windows; this row is the 2.5 Mac address of the old row)
        {3040549301u, 0x19D551C}, // FactoryIndex_ResolveResource. Windows 0x2046bf4. fork DB, verified there.
        {2923109755u, 0x442d290}, // GameApplication_InitResourceDepot. Windows 0x9f45c8. fork DB, verified there.
        {3128897273u, 0x2007EBC}, // GarmentAssemblerState_AddCustomItem. Windows 0xb56ac0. fork DB, verified there.
        {956641309u, 0x2008864}, // GarmentAssemblerState_ChangeCustomItem. Windows 0xb56a1c. R22: exact twin by the read-only call structure (four helper calls ANDed: remove by hash, the 0x9810/0xb5807c pair, the 0x8d24/0x5e9ffc one with a zero third argument, and the overrides variant with request+0x10; request hash at +0x20, index at +0x28 on both sides). The fork DB entry 0x36fe44c is a log message function.
        {3740082313u, 0x20085B4}, // GarmentAssemblerState_ChangeItem. Windows 0xb57fd8. R22: exact twin (same four calls ANDed, request hash at +0x10, index at +0x18). The fork DB entry 0x36fe44c is a log message function.
        {2294423397u, 0x2007DCC}, // GarmentAssemblerState_AddItem. Windows 0x5e9e00. fork DB, verified there.
        {809178766u, 0x17EE34C}, // GarmentAssembler_ExtractComponentsJob. Windows 0x27399c. R22 code twin: the Windows body is the second stage 0xAE6660 (the fork DB value 0xAE6348 is stage 1). Both Mac stages free their closure at the end, so the HookAfter handler cannot run; the row stays held back (NOT_YET.md).
        {2594581880u, 0x2007BC4}, // GarmentAssembler_FindState. Windows 0x5eb300. fork DB, verified there.
        {709304039u, 0x2030D18}, // GarmentAssembler_OnGameDetach. Windows 0x70437c. R22 code twin: void(this, a2), called only from the entity appearance system's detach virtual (Windows 0x557D98 / Mac 0x3712FF8). The fork DB value 0x36F6F4C is the callee that clears the per-entity states.
        {29053464u, 0x17EBFF0}, // GarmentAssembler_ProcessGarment. Windows 0xb57c60. fork DB, verified there.
        {1567972572u, 0x17EBC80}, // GarmentAssembler_ProcessMorphedMesh. Windows 0x79696c. fork DB, verified there.
        {1663588463u, 0x17EB4AC}, // GarmentAssembler_ProcessSkinnedMesh. Windows 0x7974b0. fork DB, verified there.
        {1863723270u, 0x2008310}, // GarmentAssembler_RemoveItem. Windows 0xb575bc. fork DB, verified there.
        {1828854026u, 0x19545C8}, // IPlacedComponent_SetTransform. Windows 0x574fc8. Codeware/SDK row.
        {2698985195u, 0x4368F34}, // InkSpawner_FinishAsyncSpawn. Windows 0xfa63f0. fork DB, verified there.
        {4097515464u, 0x4368F34}, // InkSpawner_FinishAsyncSpawn: the 2.5 hash of the same function (psiberx/cp2077-archive-xl#31 re-derived it on Windows; this row is the 2.5 Mac address of the old row)
        {1396063719u, 0x4353D74}, // InkWidgetLibrary_AsyncSpawnFromExternal. Windows 0x9fb2a8. fork DB, verified there.
        {225197914u, 0x4353D74}, // InkWidgetLibrary_AsyncSpawnFromExternal: the 2.5 hash of the same function (psiberx/cp2077-archive-xl#31 re-derived it on Windows; this row is the 2.5 Mac address of the old row)
        {118698863u, 0x4353a7c}, // InkWidgetLibrary_AsyncSpawnFromLocal. Windows 0x57f26c. fork DB, verified there.
        {2342073570u, 0x4353a7c}, // InkWidgetLibrary_AsyncSpawnFromLocal: the 2.5 hash of the same function (psiberx/cp2077-archive-xl#31 re-derived it on Windows; this row is the 2.5 Mac address of the old row)
        {506278179u, 0x435424C}, // InkWidgetLibrary_SpawnFromExternal. Windows 0x9fb618. fork DB, verified there.
        {1158555307u, 0x435413C}, // InkWidgetLibrary_SpawnFromLocal. Windows 0x57f9c0. fork DB, verified there.
        {3392610574u, 0x200CD00}, // ItemFactoryAppearanceChangeRequest_LoadAppearance. Windows 0x12e1a38. fork DB, verified there.
        {1291460507u, 0x200C940}, // ItemFactoryAppearanceChangeRequest_LoadTemplate. Windows 0x1387648. fork DB, verified there.
        {3659799256u, 0x20133F4}, // ItemFactoryRequest_LoadAppearance. Windows 0x5ebbf0. fork DB, verified there.
        {1576079097u, 0x4DDF5C0}, // JobHandle_Wait. Windows 0x2ac294. fork DB, verified there.
        {1964512489u, 0x5240250}, // JournalManager_LoadJournal. Windows 0x9522c0. fork DB, verified there.
        {3274382593u, 0x5243170}, // JournalManager_TrackQuest. Windows 0x5944fc. fork DB, verified there.
                {1945256833u, 0x520854C}, // JournalRootFolderEntry_Initialize. Windows 0x950620. The virtual (this, a2, a3, JobQueue) that calls the loader half 0x1E7528C and tail-branches to 0x1E75660, as the Windows virtual 0x950620 calls 0x9507fc then jumps to 0x950660. R22: the fork DB row 0x1E7528C is only the first half.
        {837162664u, 0x52360B4}, // JournalTree_ProcessJournalIndex. Windows 0x94ffcc. fork DB, verified there.
        {1488657506u, 0x3F2BD00}, // Localization_LoadLipsyncs. Windows 0x112cb18. fork DB, verified there.
        {3550098299u, 0x1C2B6DC}, // Localization_LoadOnScreens. Windows 0x261018. fork DB, verified there.
        {772484645u, 0x1C2B4C8}, // Localization_LoadSubtitles. Windows 0x262090. fork DB, verified there.
        {4223669659u, 0x3fe6c94}, // Localization_LoadVoiceOvers. Windows 0x263124. fork DB, verified there.
        {240386859u, 0x323A0}, // Main. Windows 0x18d4dc0. Codeware/SDK row.
        {3299551353u, 0x30F7C40}, // MappinSystem_GetMappinData. Windows 0x9067a0 (reads this+0x68). R22: the fork DB row 0x42A3E38 is the crossed twin; the Mac wrapper that reads this+0x68 is 0x42A3E68. Not hooked (16 byte wrapper), see FindPoiData.
        // 2.5: MappinSystem_GetPoiData (620961393) has no standalone function any more (the 4-instruction wrapper was merged into
        // 0x30F7934); it is not hooked on Mac, so the row is dropped and the hash resolves to its trap stub.
        // was {620961393u, 0x42A3E38}, // MappinSystem_GetPoiData. Windows 0x69ba6c (reads this+0x58). R22: crossed in the fork DB; the Mac wrapper that reads this+0x58 is 0x42A3E38. Not hooked (16 byte wrapper), see FindMappinData.
        {3235778561u, 0x30F2FF0}, // MappinSystem_FindMappinData (Mac-only id). R22: the finder (resource +0x40 array, count +0x4C, stride 0x20, u32 key) behind the 0x42A3E38 wrapper; 72 byte leaf, also called directly by the inlined position lookup 0x42B2748.
        {3235778562u, 0x31138F4}, // MappinSystem_FindPoiData (Mac-only id). R22: the same finder shape for the POI resource, behind the 0x42A3E68 wrapper.
        {3235778563u, 0x519AEBC}, // CMesh_PreloadAppearances (Mac-only id). R22: the action CMesh::PostLoad takes after its inlined ShouldPreloadAppearances test; (CMesh*, w1 = 0, request*), void, single caller at 0xE16D00. Windows twin 0xAF3CC4 (caller 0x224907).
        {3235778564u, 0x200857C}, // GarmentAssembler_ChangeItem (Mac-only id). R22: aggregator level wrapper bool(aggregator, WeakHandle<Entity>&, GarmentItemChangeRequest&), called once from 0x36FDDD4 (twin of the Windows request Apply 0xB57D10). The state level body 0x36F887C has no callers.
        {3235778565u, 0x200882C}, // GarmentAssembler_ChangeCustomItem (Mac-only id). R22: the same for the custom request, called once from 0x36FDDD4.
        {3235778566u, 0x26050F8}, // AppearanceNameVisualTagsPreset_GetVisualTagsByRecord (Mac-only id). R22: twin of the Windows wrapper 0x3F12F0 (record based entry), the map find is inlined and it does not call 0x3CD7234. R26: the TweakDBID field it reads at +0xE4 holds the game's cached flat offset (see GarmentExtension ResolveRecordFlatPath); the Windows wrapper reads +0xE8.
        {3235778567u, 0x1EA5F80}, // TPPRepresentationComponent_UnregisterAffectedItem (Mac-only id). R22: void(TPPRepresentationComponent*, TweakDBID aItemID), the cold call of OnItemUnequipped for an affected slot; Windows twin 0x25050CC.
        {787099588u, 0x4BC5098}, // CharacterCustomizationSystem_EnsureState. Windows 0xeed694 (cold half 0x1fc6c0a). R22 code twin: (System*, Handle<State>*), one of the three sites that inline GetResource on the Mac (the GetResource row 0x2459710 has no callers).
        {140387944u, 0x30FB9F0}, // MappinSystem_OnStreamingWorldLoaded. Windows 0x1636a4. fork DB, verified there.
        {1419388740u, 0x51A3C24}, // MeshAppearance_LoadMaterialSetupAsync. Windows 0x21a670. fork DB, verified there.
        {1609519068u, 0x51AD0E8}, // MeshMaterialBuffer_LoadMaterialAsync. Windows 0x2781a0. fork DB, verified there.
        {633871992u, 0x197F17C}, // MorphTargetManager_ApplyMorphTarget. Windows 0x1607fc. Codeware/SDK row.
        {1523127443u, 0x51CA1A4}, // MorphTargetMesh_PostLoad. Windows 0xc27adc. fork DB, verified there.
        {546121377u, 0x2D0D090}, // PersistencySystem_SetPersistentStateData. Windows 0x2494e0. fork DB, verified there.
        {790570700u, 0x29E8C48}, // QuestLoader_ProcessPhaseResource. Windows 0x9461ac. fork DB, verified there.
        {797843833u, 0x2811DF0}, // QuestRootInstance_Start. Windows 0x270026c (the virtual with the post-create helper). R22: the fork DB row 0x2D01878 is the twin of the other Windows function 0x11FC720 (not virtual, no helper); the virtual is 0x2D00870 (questRootInstance vtable slot 0xE8), with the post-create helper 0x2CFACF4.
        {2048921710u, 0x2A0B26C}, // QuestsSystem_OnGameRestored. Windows 0x495fd0. fork DB, verified there.
        {43194193u, 0x151AA2C}, // ResourceDepot_CheckResource. Windows 0x26475c. fork DB, verified there.
        {2885423437u, 0x151A118}, // ResourceDepot_InitializeArchives. Windows 0xe60e24. fork DB, verified there.
        {2892763485u, 0x151A118}, // ResourceDepot_InitializeArchives: the 2.5 hash of the same function (psiberx/cp2077-archive-xl#31 re-derived it on Windows; this row is the 2.5 Mac address of the old row)
        {2517385486u, 0x151B18C}, // ResourceDepot_LoadArchives. Windows 0x7bba48. fork DB, verified there.
        {3573367139u, 0x151B18C}, // ResourceDepot_LoadArchives: the 2.5 hash of the same function (psiberx/cp2077-archive-xl#31 re-derived it on Windows; this row is the 2.5 Mac address of the old row)
        {2450934495u, 0x151A748}, // ResourceDepot_RequestResource. Windows 0x1cfa8c. fork DB, verified there.
        {2365013187u, 0x10E710C}, // ResourceLoader_RequestResource. Windows 0x1de50c. fork DB, verified there.
        {3998356057u, 0x106C8C4}, // ResourcePath_Create. Windows 0x1df644. fork DB, verified there.
        {2901686778u, 0x120AFF4}, // ResourceSerializer_Deserialize. Windows 0x148b70. fork DB, verified there.
        {2577814646u, 0x1207588}, // ResourceSerializer_Load. Windows 0x2357e8. fork DB, verified there.
        {1185093671u, 0x12090B0}, // ResourceSerializer_OnDependenciesReady. Windows 0x24486c. fork DB, verified there.
        {3892979844u, 0x12090B0}, // ResourceSerializer_OnDependenciesReady: the 2.5 hash of the same function (psiberx/cp2077-archive-xl#31 re-derived it on Windows; this row is the 2.5 Mac address of the old row)
        {3972601611u, 0x1c05360}, // StreamingSector_PostLoad. Windows 0x26eeb4. fork DB, verified there.
        {410718963u, 0x1C09804}, // StreamingWorld_Serialize. Windows 0x113a890. fork DB, verified there.
        {4129169021u, 0x1E9FF18}, // TPPRepresentationComponent_OnAttach. Windows 0xac58c0. fork DB, verified there.
        {4010810747u, 0x1EA5B54}, // TPPRepresentationComponent_OnItemEquipped. Windows 0x9fe5f4. fork DB, verified there.
        {1933319146u, 0x1EA50BC}, // TPPRepresentationComponent_OnItemUnequipped. Windows 0x9fe1dc. fork DB, verified there.
        {3037343626u, 0x1EA0738}, // TPPRepresentationComponent_RegisterAffectedItem. Windows 0x9fdce8. fork DB, verified there.
        {3602585178u, 0x37CE05C}, // TweakDB_Load. Windows 0x50eb1c. fork DB, verified there.
        {63182599u, 0x37CE05C}, // TweakDB_Load: the 2.5 hash of the same function (psiberx/cp2077-archive-xl#31 re-derived it on Windows; this row is the 2.5 Mac address of the old row)
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
        {1693194103u, 0x43181C8}, // InkWorldLayer_UpdateComponents. Windows 0x29eebc. R22 code twin: no standalone function on the Mac; the lambda is inlined into the parallel-for job body 0x491DDB0 (Windows job body 0xC51EEC). The limit is `cmp x22,#9; b.hi` at +0x12C/+0x130. The row is read as code (WorldWidgetLimitPatch), never called. The fork DB entry 0x3da0574 was unverified.
        {2254060789u, 0x43181C8}, // InkWorldLayer_UpdateComponents: the 2.5 hash of the same function (psiberx/cp2077-archive-xl#31 re-derived it on Windows; this row is the 2.5 Mac address of the old row)
        {1303056161u, 0x10E4FD0}, // ResourceLoader_OnUpdate. Windows 0x270acc. R22 code twin: void(ResourceLoader*), the loader's per-frame token cache maintenance, one caller (the tail call wrapper 0x21B99A8, called once per frame from 0x3D99938 like Windows 0x29329C from 0x291DF0). The fork DB entry 0x2d9766c was unverified.
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
