#include "Extension.hpp"
#include "App/Extensions/Attachment/Extension.hpp"
#include "App/Extensions/Customization/Extension.hpp"
#include "App/Shared/ResourcePathRegistry.hpp"
#include "Core/Facades/Container.hpp"
#include "Red/Entity.hpp"
#include "Red/TweakDB.hpp"
#if defined(__APPLE__)
#include "App/Utils/MacProbe.hpp"
#include "MacAddressResolver.hpp"
#include "MacDiag.hpp"
#include <atomic>
#include <cstring>
#endif

namespace
{
constexpr auto ExtensionName = "GarmentOverride";

constexpr auto BodyPartTag = Red::CName("PlayerBodyPart");

constexpr auto DefaultAppearanceName = Red::CName("default");
constexpr auto EmptyAppearanceName = Red::CName("empty_appearance_default");
constexpr auto EmptyAppearancePath = Red::ResourcePath(R"(base\characters\appearances\player\items\empty_appearance.app)");
constexpr auto RandomAppearanceName = Red::CName("random");

constexpr auto EmptyAppearanceTags = {
    std::pair(Red::CName("EmptyAppearance:FPP"), "&FPP"),
    std::pair(Red::CName("EmptyAppearance:Male"), "&Male"),
    std::pair(Red::CName("EmptyAppearance:Female"), "&Female"),
};

constexpr auto AggregatedHeadAppearanceName = Red::CName("AppearanceName1");
constexpr auto DummyAppearancePartPath = Red::ResourcePath(R"(#non_existing_part.ent)");

Red::SharedSpinLock s_dynamicTagsLock;
Core::Map<uint64_t, Core::Vector<Red::CName>> s_dynamicTagsCache;
}

std::string_view App::GarmentExtension::GetName()
{
    return ExtensionName;
}

bool App::GarmentExtension::Load()
{
    HookBefore<Raw::ItemFactoryRequest::LoadAppearance>(&OnLoadAppearanceResource).OrThrow();
    HookBefore<Raw::ItemFactoryAppearanceChangeRequest::LoadAppearance>(&OnChangeAppearanceResource).OrThrow();
    Hook<Raw::EntityTemplate::FindAppearance>(&OnResolveAppearance).OrThrow();
#if defined(__APPLE__)
    HookAfter<Raw::AppearanceResource::FindAppearance>(&OnFindDefinition).OrThrow();
#else
    HookAfter<Raw::AppearanceResource::FindAppearance>(&OnResolveDefinition).OrThrow();
#endif
    Hook<Raw::AppearanceChanger::GetSuffixes>(&OnResolveSuffixes).OrThrow();
    HookAfter<Raw::AppearanceNameVisualTagsPreset::GetVisualTags>(&OnGetVisualTags).OrThrow();
#if defined(__APPLE__)
    HookAfter<Raw::AppearanceNameVisualTagsPreset::GetVisualTagsByRecord>(&OnGetVisualTagsByRecord).OrThrow();
    HookAfter<Raw::GarmentAssembler::FindState>(&OnFindStateResult).OrThrow();
#else
    HookAfter<Raw::GarmentAssembler::FindState>(&OnFindState).OrThrow();
#endif
    HookBefore<Raw::GarmentAssemblerState::AddItem>(&OnAddItem).OrThrow();
    HookBefore<Raw::GarmentAssemblerState::AddCustomItem>(&OnAddCustomItem).OrThrow();
#if defined(__APPLE__)
    HookBefore<Raw::GarmentAssembler::ChangeItem>(&OnChangeItem).OrThrow();
    HookBefore<Raw::GarmentAssembler::ChangeCustomItem>(&OnChangeCustomItem).OrThrow();
#else
    HookBefore<Raw::GarmentAssemblerState::ChangeItem>(&OnChangeItem).OrThrow();
    HookBefore<Raw::GarmentAssemblerState::ChangeCustomItem>(&OnChangeCustomItem).OrThrow();
#endif
    HookBefore<Raw::GarmentAssembler::RemoveItem>(&OnRemoveItem).OrThrow();
    Hook<Raw::GarmentAssembler::ProcessGarment>(&OnProcessGarment).OrThrow();
    HookWrap<Raw::GarmentAssembler::ProcessSkinnedMesh>(&OnProcessGarmentMesh).OrThrow();
    HookWrap<Raw::GarmentAssembler::ProcessMorphedMesh>(&OnProcessGarmentMesh).OrThrow();
    HookBefore<Raw::GarmentAssembler::OnGameDetach>(&OnGameDetach).OrThrow();
    HookBefore<Raw::AppearanceChanger::RegisterPart>(&OnRegisterPart).OrThrow();
    //Hook<Raw::AppearanceChanger::GetBaseMeshOffset>(&OnGetBaseMeshOffset).OrThrow();
    HookBefore<Raw::AppearanceChanger::ComputePlayerGarment>(&OnComputeGarment).OrThrow();
    HookBefore<Raw::Entity::ReassembleAppearance>(&OnReassembleAppearance).OrThrow();

#if defined(__APPLE__)
    MacProbe::AddStats(&LogMacStats);
#endif

    s_emptyAppearance = Core::MakeUnique<Red::TemplateAppearance>();
    s_emptyAppearance->name = EmptyAppearanceName;
    s_emptyAppearance->appearanceName = DefaultAppearanceName;
    s_emptyAppearance->appearanceResource = EmptyAppearancePath;

    s_dynamicAppearance = Core::MakeShared<DynamicAppearanceController>(Core::Resolve<ResourcePathRegistry>());
    s_stateManager = Core::MakeUnique<OverrideStateManager>(s_dynamicAppearance);
    s_tagManager = Core::MakeShared<OverrideTagManager>();

    return true;
}

bool App::GarmentExtension::Unload()
{
    Unhook<Raw::ItemFactoryRequest::LoadAppearance>();
    Unhook<Raw::ItemFactoryAppearanceChangeRequest::LoadAppearance>();
    Unhook<Raw::EntityTemplate::FindAppearance>();
    Unhook<Raw::AppearanceResource::FindAppearance>();
    Unhook<Raw::AppearanceChanger::GetSuffixes>();
    Unhook<Raw::AppearanceNameVisualTagsPreset::GetVisualTags>();
#if defined(__APPLE__)
    Unhook<Raw::AppearanceNameVisualTagsPreset::GetVisualTagsByRecord>();
#endif
    Unhook<Raw::GarmentAssembler::FindState>();
    Unhook<Raw::GarmentAssemblerState::AddItem>();
    Unhook<Raw::GarmentAssemblerState::AddCustomItem>();
#if defined(__APPLE__)
    Unhook<Raw::GarmentAssembler::ChangeItem>();
    Unhook<Raw::GarmentAssembler::ChangeCustomItem>();
#else
    Unhook<Raw::GarmentAssemblerState::ChangeItem>();
    Unhook<Raw::GarmentAssemblerState::ChangeCustomItem>();
#endif
    Unhook<Raw::GarmentAssembler::RemoveItem>();
    Unhook<Raw::GarmentAssembler::ProcessGarment>();
    Unhook<Raw::GarmentAssembler::ProcessSkinnedMesh>();
    Unhook<Raw::GarmentAssembler::ProcessMorphedMesh>();
    Unhook<Raw::GarmentAssembler::OnGameDetach>();
    Unhook<Raw::AppearanceChanger::RegisterPart>();
    // Unhook<Raw::AppearanceChanger::GetBaseMeshOffset>();
    Unhook<Raw::AppearanceChanger::ComputePlayerGarment>();
    Unhook<Raw::Entity::ReassembleAppearance>();

    s_dynamicAppearance.reset();
    s_stateManager.reset();
    s_tagManager.reset();

    return true;
}

#if defined(__APPLE__)
void App::GarmentExtension::OnTweakDBReady()
{
    DynamicAppearanceController::CreateSuffixProbes();
}

// R26. The flat that a record's TweakDBID field names, read the way the game reads it.
//
// A TweakDBID field inside a record is not a plain id. As the game keeps it (Mac 0x9C8520, the twin of Windows 0x3F1CAC) it is
//   +0 hash, +4 length, +5..+7 the flat offset that the game caches there (big endian, from the flat data buffer at TweakDB+0, pointing at the
//   VALUE of the flat), +8 a lock byte, +9 the TweakDB version byte (TweakDB+0x160, Mac accessor 0x2B743C0) that the cached offset was made under.
// The game's fast path returns buffer + offset when the version byte matches and resolves the offset again (slow path 0x9C9798) when it does not.
// The SDK's GetFlatValue takes a TweakDBID that has an offset for the id of a FlatValue, so a field passed to it as it is calls a virtual function
// through the first 8 bytes of the value (r26 crash on the first save load, GetFlatPtr+216).
//
// The by-record hook is a HookAfter: the original has just run 0x9C8520 on this very field, so the cache is fresh and this takes the same pointer the game
// used. When the version byte does not match (the table changed in between) the flat is looked up by hash and length alone, as every other lookup of
// the plugin does.
// Diagnostics only (CODEWARE_MAC_DIAG=1): how the calls of the by-record hook were resolved, and whether the pointer taken from the game's cache is the one
// the plugin's own lookup by hash and length finds.
static std::atomic<uint32_t> s_recordCalls{0}, s_recordCached{0}, s_recordLookup{0}, s_recordUnresolved{0}, s_recordSame{0}, s_recordDiffer{0};

static const Red::ResourcePath* ResolveRecordFlatPath(const uint8_t* aField)
{
    auto* tweakDB = Red::TweakDB::Get();
    if (!tweakDB)
        return nullptr;

    uint64_t raw = 0;
    std::memcpy(&raw, aField, sizeof(raw));
    const Red::TweakDBID field(raw);
    const Red::TweakDBID clean(field.name.hash, field.name.length);
    const bool diag = Mac::DiagEnabled();
    if (diag)
        s_recordCalls.fetch_add(1, std::memory_order_relaxed);

    if (aField[9] == tweakDB->unk160 && field.HasTDBOffset() && tweakDB->staticFlatDataBuffer)
    {
        const auto* cached = reinterpret_cast<const Red::ResourcePath*>(tweakDB->staticFlatDataBuffer + static_cast<uint32_t>(field.ToTDBOffset()));
        if (diag)
        {
            s_recordCached.fetch_add(1, std::memory_order_relaxed);
            const auto* viaTable = Red::GetFlatPtr<Red::ResourcePath>(clean);
            if (viaTable == cached)
            {
                s_recordSame.fetch_add(1, std::memory_order_relaxed);
            }
            else if (s_recordDiffer.fetch_add(1, std::memory_order_relaxed) < 8)
            {
                MAC_DIAG("[selftest] R26 by-record differ: id %08x/%u cached offset 0x%06x -> %p (hash %016llx), table lookup %p\n", clean.name.hash,
                         static_cast<unsigned>(clean.name.length), static_cast<unsigned>(field.ToTDBOffset()), static_cast<const void*>(cached),
                         static_cast<unsigned long long>(*reinterpret_cast<const uint64_t*>(cached)), static_cast<const void*>(viaTable));
            }
        }
        return cached;
    }

    const auto* viaTable = Red::GetFlatPtr<Red::ResourcePath>(clean);
    if (diag)
        (viaTable ? s_recordLookup : s_recordUnresolved).fetch_add(1, std::memory_order_relaxed);
    return viaTable;
}

// R26 self-test (diagnostics only, once at t+50s): what ResolveRecordFlatPath relies on, shown on the live TweakDB. Nothing here needs a record: the
// game's own accessor of the version byte (0x2B743C0, a leaf) against the SDK member at +0x160, the buffer base at TweakDB+0 against the SDK's
// flatDataBuffer, and, for a sample of the flats table, that the SDK's value pointer is the table offset plus the data member (the offset the game
// caches in a record is the offset of the value, not of the FlatValue).
static void ByRecordFlatSelfTest()
{
    static bool s_done = false;
    if (s_done)
        return;
    s_done = true;

    auto* tweakDB = Red::TweakDB::Get();
    if (!tweakDB)
        return;

    using VersionAccessor = uint8_t (*)();
    const auto gameVersion = reinterpret_cast<VersionAccessor>(Red::MacGameBase() + 0x2B743C0)();
    const bool versionOK = gameVersion == tweakDB->unk160;
    const bool bufferOK = tweakDB->staticFlatDataBuffer != 0 && tweakDB->staticFlatDataBuffer == tweakDB->flatDataBuffer;

    const uint32_t count = tweakDB->flats.size;
    unsigned consistent = 0, inconsistent = 0;
    for (uint32_t k = 0; k < 16 && count > 16; ++k)
    {
        const uint32_t index = static_cast<uint32_t>((static_cast<uint64_t>(count) * k) / 16) + 7;
        const Red::TweakDBID entry = tweakDB->flats.entries[index];
        const Red::TweakDBID clean(entry.name.hash, entry.name.length);
        auto* flat = tweakDB->GetFlatValue(clean);
        const uintptr_t flatPtr = reinterpret_cast<uintptr_t>(flat);
        const uintptr_t valuePtr = flat ? reinterpret_cast<uintptr_t>(flat->GetValue().value) : 0;
        const uintptr_t tablePtr = tweakDB->staticFlatDataBuffer + static_cast<uint32_t>(entry.ToTDBOffset());
        const bool ok = flat && flatPtr == tablePtr && valuePtr > flatPtr && valuePtr - flatPtr <= 0x20;
        consistent += ok ? 1 : 0;
        inconsistent += ok ? 0 : 1;
        MAC_DIAG("[selftest] R26 by-record flat sample %u: index %u id %08x/%u table offset 0x%06x -> %p, SDK FlatValue %p, value %p (+0x%x) %s\n", k,
                 index, entry.name.hash, static_cast<unsigned>(entry.name.length), static_cast<unsigned>(entry.ToTDBOffset()),
                 reinterpret_cast<void*>(tablePtr), reinterpret_cast<void*>(flatPtr), reinterpret_cast<void*>(valuePtr),
                 static_cast<unsigned>(valuePtr - flatPtr), ok ? "consistent" : "INCONSISTENT");
    }
    MAC_DIAG("[selftest] R26 by-record flat: version byte SDK 0x%02x game 0x%02x %s; buffer base %p flatDataBuffer %p %s; %u of %u samples consistent %s\n",
             static_cast<unsigned>(tweakDB->unk160), static_cast<unsigned>(gameVersion), versionOK ? "same" : "DIFFERENT",
             reinterpret_cast<void*>(tweakDB->staticFlatDataBuffer), reinterpret_cast<void*>(tweakDB->flatDataBuffer), bufferOK ? "same" : "DIFFERENT",
             consistent, consistent + inconsistent,
             (versionOK && bufferOK && inconsistent == 0 && consistent > 0) ? "PASS" : "FAIL");
}

void App::GarmentExtension::LogMacStats(const char* aWhere)
{
    DynamicAppearanceController::LogMacStats(aWhere);
    if (Mac::DiagEnabled() && std::strcmp(aWhere, "t+50s") == 0)
        ByRecordFlatSelfTest();
    if (Mac::DiagEnabled())
    {
        const auto calls = s_recordCalls.load(), cached = s_recordCached.load(), lookup = s_recordLookup.load(), unresolved = s_recordUnresolved.load();
        const auto same = s_recordSame.load(), differ = s_recordDiffer.load();
        MAC_DIAG("[selftest] R26 by-record calls %s: %u calls, %u resolved through the cached offset, %u through the lookup by hash and length, %u unresolved; the cached "
                 "pointer equals the table lookup in %u of %u compared (%u differ) %s\n",
                 aWhere, calls, cached, lookup, unresolved, same, same + differ, differ, (unresolved == 0 && differ == 0) ? "PASS" : "FAIL");
    }

    MAC_DIAG("[selftest] R22 Garment %s: hooks ran: item request %u, change request %u, template %u, definition %u, suffixes %u, visual tags %u (by record %u), "
             "find state %u, add item %u, add custom %u, change item %u, change custom %u, remove %u, register part %u, process garment %u, "
             "process mesh %u, compute garment %u, reassemble %u, game detach %u\n",
             aWhere, s_hits[HitLoadAppearance].load(), s_hits[HitChangeAppearance].load(), s_hits[HitResolveAppearance].load(),
             s_hits[HitResolveDefinition].load(), s_hits[HitSuffixes].load(), s_hits[HitVisualTags].load(), s_hits[HitVisualTagsByRecord].load(),
             s_hits[HitFindState].load(), s_hits[HitAddItem].load(), s_hits[HitAddCustomItem].load(), s_hits[HitChangeItem].load(),
             s_hits[HitChangeCustomItem].load(), s_hits[HitRemoveItem].load(), s_hits[HitRegisterPart].load(), s_hits[HitProcessGarment].load(),
             s_hits[HitProcessMesh].load(), s_hits[HitComputeGarment].load(), s_hits[HitReassemble].load(), s_hits[HitGameDetach].load());
}
#endif

void App::GarmentExtension::Configure()
{
    std::unique_lock _(s_mutex);
    for (const auto& unit : m_configs)
    {
        for (const auto& tag : unit.tags)
        {
            s_tagManager->DefineTag(tag.first.data(), tag.second);
        }
    }
}

void App::GarmentExtension::OnLoadAppearanceResource(Red::ItemFactoryRequest* aRequest)
{
#if defined(__APPLE__)
    Mac::Count(s_hits[HitLoadAppearance]);
#endif
    auto& entityWeak = Raw::ItemFactoryRequest::Entity::Ref(aRequest);
    auto& templateToken = Raw::ItemFactoryRequest::EntityTemplate::Ref(aRequest);
    auto& appearanceName = Raw::ItemFactoryRequest::AppearanceName::Ref(aRequest);
    auto itemRecord = Raw::ItemFactoryRequest::ItemRecord::Ref(aRequest);

    if (!appearanceName && itemRecord)
    {
        appearanceName = Red::GetFlatValue<Red::CName>({itemRecord->recordID, ".appearanceName"});
    }

    if (PrepareDynamicAppearanceName(entityWeak, templateToken, appearanceName))
    {
        std::unique_lock _(s_mutex);
        if (auto& entityState = s_stateManager->GetEntityState(entityWeak.instance))
        {
#ifndef NDEBUG
            LogDebug("[{}] [event=LoadAppearanceResource entity={}]", ExtensionName, entityState->GetName());
#endif
            if (itemRecord)
            {
                UpdateDynamicAttributes(entityState, itemRecord->recordID);
            }
            else
            {
                UpdateDynamicAttributes(entityState);
            }
        }
    }
}

void App::GarmentExtension::OnChangeAppearanceResource(Red::ItemFactoryAppearanceChangeRequest* aRequest)
{
#if defined(__APPLE__)
    Mac::Count(s_hits[HitChangeAppearance]);
#endif
    auto& entityWeak = Raw::ItemFactoryAppearanceChangeRequest::Entity::Ref(aRequest);
    auto& templateToken = Raw::ItemFactoryAppearanceChangeRequest::EntityTemplate::Ref(aRequest);
    auto& appearanceName = Raw::ItemFactoryAppearanceChangeRequest::AppearanceName::Ref(aRequest);
    auto itemRecord = Raw::ItemFactoryAppearanceChangeRequest::ItemRecord::Ref(aRequest);

    if (!appearanceName && itemRecord)
    {
        appearanceName = Red::GetFlatValue<Red::CName>({itemRecord->recordID, ".appearanceName"});
    }

    if (PrepareDynamicAppearanceName(entityWeak, templateToken, appearanceName))
    {
        std::unique_lock _(s_mutex);
        if (auto& entityState = s_stateManager->GetEntityState(entityWeak.instance))
        {
#ifndef NDEBUG
            LogDebug("[{}] [event=ChangeAppearanceResource entity={}]", ExtensionName, entityState->GetName());
#endif
            if (itemRecord)
            {
                UpdateDynamicAttributes(entityState, itemRecord->recordID);
            }
            else
            {
                UpdateDynamicAttributes(entityState);
            }
        }
    }
}

Red::TemplateAppearance* App::GarmentExtension::OnResolveAppearance(Red::EntityTemplate* aTemplate,
                                                                         Red::CName aSelector)
{
#if defined(__APPLE__)
    Mac::Count(s_hits[HitResolveAppearance]);
#endif
    if (aSelector == EmptyAppearanceName)
        return s_emptyAppearance.get();

    auto* appearance = Raw::EntityTemplate::FindAppearance(aTemplate, aSelector);

    if (!IsUniqueAppearanceName(aSelector))
        return appearance;

    if (s_dynamicAppearance->SupportsDynamicAppearance(aTemplate))
    {
        auto selector = s_dynamicAppearance->ParseAppearance(aSelector);
        if (selector.isDynamic)
        {
            if (selector.context)
            {
                std::unique_lock _(s_mutex);
                if (auto& entityState = s_stateManager->GetEntityState(selector.context))
                {
                    SelectDynamicAppearance(entityState, selector, aTemplate, appearance);
                    return appearance;
                }
            }
            else
            {
                std::unique_lock _(s_mutex);
                if (auto& entityState = s_stateManager->GetFirstEntityState())
                {
                    SelectDynamicAppearance(entityState, selector, aTemplate, appearance);
                    return appearance;
                }
            }
        }
    }

    if (aTemplate->visualTagsSchema && !aTemplate->visualTagsSchema->visualTags.IsEmpty())
    {
        const auto& visualTags = aTemplate->visualTagsSchema->visualTags;
        std::string_view selectorStr = aSelector.ToString();

        for (const auto& tag : EmptyAppearanceTags)
        {
            if (visualTags.Contains(tag.first) && selectorStr.find(tag.second) != std::string_view::npos)
            {
                {
                    std::unique_lock _(s_mutex);
                    aTemplate->appearances.EmplaceBack(*s_emptyAppearance);
                    appearance = &aTemplate->appearances.Back();
                }

                appearance->name = aSelector;
                return appearance;
            }
        }
    }

    return appearance;
}

#if defined(__APPLE__)
void App::GarmentExtension::OnFindDefinition(Red::Handle<Red::AppearanceDefinition>& aDefinition,
                                             Red::AppearanceResource* aResource, Red::CName aSelector, uint32_t a4,
                                             uint8_t a5)
{
    OnResolveDefinition(aResource, &aDefinition, aSelector, a4, a5);
}
#endif

void App::GarmentExtension::OnResolveDefinition(Red::AppearanceResource* aResource,
                                                     Red::Handle<Red::AppearanceDefinition>* aDefinition,
                                                     Red::CName aSelector, uint32_t a4, uint8_t a5)
{
#if defined(__APPLE__)
    Mac::Count(s_hits[HitResolveDefinition]);
#endif
    if (!*aDefinition)
    {
        auto selector = s_dynamicAppearance->ParseAppearance(aSelector);
        if (selector.isDynamic)
        {
            if (selector.context)
            {
                std::unique_lock _(s_mutex);
                if (auto& entityState = s_stateManager->GetEntityState(selector.context))
                {
                    SelectDynamicAppearance(entityState, selector, aResource, *aDefinition);
                }
            }
            else
            {
                std::unique_lock _(s_mutex);
                if (auto& entityState = s_stateManager->GetFirstEntityState())
                {
                    SelectDynamicAppearance(entityState, selector, aResource, *aDefinition);
                }
            }
        }
        else
        {
            CustomizationExtension::FixCustomizationAppearance(aResource, aDefinition, aSelector);
        }
    }

    if (auto definition = aDefinition->instance)
    {
        if (!definition->partsValues.IsEmpty())
        {
            auto depot = Red::ResourceDepot::Get();
            for (auto i = static_cast<int32_t>(definition->partsValues.Size() - 1); i >= 0; --i)
            {
                if (!depot->ResourceExists(definition->partsValues[i].resource.path))
                {
                    definition->partsValues.RemoveAt(i);
                }
            }
        }
    }
}

#if defined(__APPLE__)
Red::CString App::GarmentExtension::OnResolveSuffixes(Red::Handle<Red::GameObject>& aOwner,
                                                      Red::Handle<Red::GameObject>& aOwnerOverride,
                                                      const Red::Handle<Red::TweakDBRecord>& aItemRecord,
                                                      const Red::ItemID& aItemID)
{
    Mac::Count(s_hits[HitSuffixes]);

    if (aItemRecord)
    {
        auto appearanceName = Red::GetFlatValue<Red::CName>({aItemRecord->recordID, ".appearanceName"});
        if (s_dynamicAppearance->IsDynamicAppearanceName(appearanceName))
            return {};
    }

    auto result = Raw::AppearanceChanger::GetSuffixes(aOwner, aOwnerOverride, aItemRecord, aItemID);

    // GetSuffixValue is inlined into the suffix loop of GetSuffixes on the Mac, so the inner sleeves override that the Windows build makes
    // by replacing GetSuffixValue is applied to the finished suffix string.
    if (aItemRecord)
    {
        AttachmentExtension::ApplyInnerSleevesSuffix(result, aOwner, aItemRecord->recordID, aItemID);
    }

    return result;
}
#else
void* App::GarmentExtension::OnResolveSuffixes(Red::CString& aResult,
                                                    Red::Handle<Red::GameObject>& aOwner,
                                                    Red::Handle<Red::GameObject>& aOwnerOverride,
                                                    const Red::TweakDBRecord& aItemRecord,
                                                    const Red::ItemID& aItemID)
{
    auto appearanceName = Red::GetFlatValue<Red::CName>({aItemRecord.recordID, ".appearanceName"});
    if (s_dynamicAppearance->IsDynamicAppearanceName(appearanceName))
    {
        if (aResult.Length() > 0)
        {
            aResult = {};
        }
        return &aResult;
    }

    return Raw::AppearanceChanger::GetSuffixes(aResult, aOwner, aOwnerOverride, aItemRecord, aItemID);
}
#endif

void App::GarmentExtension::OnGetVisualTags(Red::AppearanceNameVisualTagsPreset* aPreset, Red::ResourcePath aEntityPath,
                                            Red::CName aAppearanceName, Red::TagList& aFinalTags)
{
#if defined(__APPLE__)
    Mac::Count(s_hits[HitVisualTags]);
#endif
    if (!aAppearanceName || aAppearanceName == EmptyAppearanceName)
        return;

// #ifndef NDEBUG
//     if (aFinalTags.tags.IsEmpty())
//     {
//         LogDebug("[{}] [event=GetVisualTags ent={} app={}]", ExtensionName, aEntityPath.hash, aAppearanceName.ToString());
//     }
// #endif

    auto cacheKey = Red::FNV1a64(aAppearanceName.ToString(), aEntityPath.hash);

    {
        std::shared_lock _(s_dynamicTagsLock);
        const auto& cachedTagsIt = s_dynamicTagsCache.find(cacheKey);
        if (cachedTagsIt != s_dynamicTagsCache.end())
        {
            for (const auto& tag : cachedTagsIt.value())
            {
                aFinalTags.Add(tag);
            }
            return;
        }
    }

    Core::Vector<Red::CName> dynamicTags;

    auto loader = Red::ResourceLoader::Get();
    auto entityToken = loader->FindToken<Red::ent::EntityTemplate>(aEntityPath);

    if (!entityToken)
    {
        entityToken = loader->LoadAsync<Red::ent::EntityTemplate>(aEntityPath);
        Red::WaitForResource(entityToken, std::chrono::milliseconds(500));
    }

    auto entityTemplate = entityToken->Get();

    if (!entityTemplate)
    {
        for (const auto& tag : dynamicTags)
        {
            aFinalTags.Add(tag);
        }

        std::unique_lock _(s_dynamicTagsLock);
        s_dynamicTagsCache.insert_or_assign(cacheKey, std::move(dynamicTags));
        return;
    }

    if (entityTemplate->visualTagsSchema)
    {
        for (const auto& tag : entityTemplate->visualTagsSchema->visualTags.tags)
        {
            dynamicTags.push_back(tag);
        }
    }

    auto appearanceTemplate = OnResolveAppearance(entityTemplate, aAppearanceName);

    if (!appearanceTemplate)
    {
        for (const auto& tag : dynamicTags)
        {
            aFinalTags.Add(tag);
        }

        std::unique_lock _(s_dynamicTagsLock);
        s_dynamicTagsCache.insert_or_assign(cacheKey, std::move(dynamicTags));
        return;
    }

    auto appearancePath = appearanceTemplate->appearanceResource.path;
    auto appearanceToken = loader->FindToken<Red::appearance::AppearanceResource>(appearancePath);

    if (!appearanceToken)
    {
        appearanceToken = loader->LoadAsync<Red::appearance::AppearanceResource>(appearancePath);
        Red::WaitForResource(appearanceToken, std::chrono::milliseconds(500));
    }

    auto appearanceResource = appearanceToken->Get();

    if (!appearanceResource)
    {
        for (const auto& tag : dynamicTags)
        {
            aFinalTags.Add(tag);
        }

        std::unique_lock _(s_dynamicTagsLock);
        s_dynamicTagsCache.insert_or_assign(cacheKey, std::move(dynamicTags));
        return;
    }

#if defined(__APPLE__)
    auto appearanceDefinition = Raw::AppearanceResource::FindAppearance(appearanceResource,
                                                                        appearanceTemplate->appearanceName, 0, 0);
#else
    Red::Handle<Red::AppearanceDefinition> appearanceDefinition;
    Raw::AppearanceResource::FindAppearance(appearanceResource, &appearanceDefinition,
                                            appearanceTemplate->appearanceName, 0, 0);
#endif
    OnResolveDefinition(appearanceResource, &appearanceDefinition, appearanceTemplate->appearanceName, 0, 0);

    if (!appearanceDefinition)
    {
        for (const auto& tag : dynamicTags)
        {
            aFinalTags.Add(tag);
        }

        std::unique_lock _(s_dynamicTagsLock);
        s_dynamicTagsCache.insert_or_assign(cacheKey, std::move(dynamicTags));
        return;
    }

    for (const auto& tag : appearanceDefinition->visualTags.tags)
    {
        dynamicTags.push_back(tag);
    }

    for (const auto& tag : dynamicTags)
    {
        aFinalTags.Add(tag);
    }

    std::unique_lock _(s_dynamicTagsLock);
    s_dynamicTagsCache.insert_or_assign(cacheKey, std::move(dynamicTags));
}

#if defined(__APPLE__)
void App::GarmentExtension::OnGetVisualTagsByRecord(Red::AppearanceNameVisualTagsPreset* aPreset, const uint8_t* aRecord,
                                                    Red::CName aAppearanceName, Red::TagList& aFinalTags)
{
    Mac::Count(s_hits[HitVisualTagsByRecord]);

    // The record names the entity path through the flat that its TweakDBID at +0xE4 points to (the Windows wrapper reads +0xE8). R26: that field
    // carries the game's cached flat offset; ResolveRecordFlatPath reads it the way the game does (see there).
    if (!aRecord)
        return;

    const auto* entityPath = ResolveRecordFlatPath(aRecord + 0xE4);

    if (!entityPath)
        return;

    OnGetVisualTags(aPreset, *entityPath, aAppearanceName, aFinalTags);
}

void App::GarmentExtension::OnFindStateResult(Red::GarmentAssemblerState& aState, uintptr_t aAggregator,
                                              Red::WeakHandle<Red::Entity>& aEntityWeak)
{
    Mac::Count(s_hits[HitFindState]);

    OnFindState(aAggregator, &aState, aEntityWeak);
}
#endif

void App::GarmentExtension::OnFindState(uintptr_t, Red::GarmentAssemblerState* aState,
                                             Red::WeakHandle<Red::Entity>& aEntityWeak)
{
    if (aState->unk00)
    {
        if (auto entity = aEntityWeak.Lock())
        {
            std::unique_lock _(s_mutex);
            s_stateManager->LinkEntityToPointer(entity, aState->unk00);
        }
    }
}

void App::GarmentExtension::OnAddItem(Red::GarmentAssemblerState* aState,
                                           Red::GarmentItemAddRequest& aRequest)
{
#if defined(__APPLE__)
    Mac::Count(s_hits[HitAddItem]);
#endif
    std::unique_lock _(s_mutex);
    if (auto& entityState = s_stateManager->FindEntityState(aState->unk00))
    {
#ifndef NDEBUG
        LogDebug("[{}] [event=AddItem entity={} item={} app={}]",
                 ExtensionName, entityState->GetName(), aRequest.hash, aRequest.apperance->name.ToString());
#endif

        UpdatePartAttributes(entityState, aRequest.apperance);
        RegisterOffsetOverrides(entityState, aRequest.hash, aRequest.apperance, aRequest.offset);
        RegisterComponentOverrides(entityState, aRequest.hash, aRequest.apperance);
    }
}

void App::GarmentExtension::OnAddCustomItem(Red::GarmentAssemblerState* aState,
                                                 Red::GarmentItemAddCustomRequest& aRequest)
{
#if defined(__APPLE__)
    Mac::Count(s_hits[HitAddCustomItem]);
#endif
    std::unique_lock _(s_mutex);
    if (auto& entityState = s_stateManager->FindEntityState(aState->unk00))
    {
#ifndef NDEBUG
        LogDebug("[{}] [event=AddCustomItem entity={} item={} app={}]",
                 ExtensionName, entityState->GetName(), aRequest.hash, aRequest.apperance->name.ToString());
#endif

        UpdatePartAttributes(entityState, aRequest.apperance);
        RegisterOffsetOverrides(entityState, aRequest.hash, aRequest.apperance, aRequest.offset);
        RegisterComponentOverrides(entityState, aRequest.hash, aRequest.apperance);
        RegisterComponentOverrides(entityState, aRequest.hash, aRequest.overrides);
    }
}

#if defined(__APPLE__)
void App::GarmentExtension::OnChangeItem(uintptr_t, Red::WeakHandle<Red::Entity>& aEntityWeak,
                                         Red::GarmentItemChangeRequest& aRequest)
{
    Mac::Count(s_hits[HitChangeItem]);

    auto entity = aEntityWeak.Lock();
    if (!entity)
        return;

    std::unique_lock _(s_mutex);
    if (auto& entityState = s_stateManager->GetEntityState(entity))
    {
#else
void App::GarmentExtension::OnChangeItem(Red::GarmentAssemblerState* aState,
                                              Red::GarmentItemChangeRequest& aRequest)
{
    std::unique_lock _(s_mutex);
    if (auto& entityState = s_stateManager->FindEntityState(aState->unk00))
    {
#endif
#ifndef NDEBUG
        LogDebug("[{}] [event=ChangeItem entity={} item={} app={}]",
                 ExtensionName, entityState->GetName(), aRequest.hash, aRequest.apperance->name.ToString());
#endif

        UnregisterComponentOverrides(entityState, aRequest.hash);
        UpdatePartAttributes(entityState, aRequest.apperance);
        RegisterComponentOverrides(entityState, aRequest.hash, aRequest.apperance);
    }
}

#if defined(__APPLE__)
void App::GarmentExtension::OnChangeCustomItem(uintptr_t, Red::WeakHandle<Red::Entity>& aEntityWeak,
                                               Red::GarmentItemChangeCustomRequest& aRequest)
{
    Mac::Count(s_hits[HitChangeCustomItem]);

    auto entity = aEntityWeak.Lock();
    if (!entity)
        return;

    std::unique_lock _(s_mutex);
    if (auto& entityState = s_stateManager->GetEntityState(entity))
    {
#else
void App::GarmentExtension::OnChangeCustomItem(Red::GarmentAssemblerState* aState,
                                                    Red::GarmentItemChangeCustomRequest& aRequest)
{
    std::unique_lock _(s_mutex);
    if (auto& entityState = s_stateManager->FindEntityState(aState->unk00))
    {
#endif
#ifndef NDEBUG
        LogDebug("[{}] [event=ChangeCustomItem entity={} item={} app={}]",
                 ExtensionName, entityState->GetName(), aRequest.hash, aRequest.apperance->name.ToString());
#endif

        UnregisterComponentOverrides(entityState, aRequest.hash);
        UpdatePartAttributes(entityState, aRequest.apperance);
        RegisterComponentOverrides(entityState, aRequest.hash, aRequest.apperance);
        RegisterComponentOverrides(entityState, aRequest.hash, aRequest.overrides);
    }
}

void App::GarmentExtension::OnRemoveItem(uintptr_t, Red::WeakHandle<Red::Entity>& aEntityWeak,
                                              Red::GarmentItemRemoveRequest& aRequest)
{
#if defined(__APPLE__)
    Mac::Count(s_hits[HitRemoveItem]);
#endif
    if (auto entity = aEntityWeak.Lock())
    {
        std::unique_lock _(s_mutex);
        if (auto& entityState = s_stateManager->GetEntityState(entity))
        {
#ifndef NDEBUG
            LogDebug("[{}] [event=RemoveItem entity={} item={}]",
                     ExtensionName, entityState->GetName(), aRequest.hash);
#endif

            UnregisterOffsetOverrides(entityState, aRequest.hash);
            UnregisterComponentOverrides(entityState, aRequest.hash);
        }
    }
}

void App::GarmentExtension::OnRegisterPart(uintptr_t, Red::Handle<Red::EntityTemplate>& aPart,
                                                Red::Handle<Red::ComponentsStorage>& aComponentStorage,
                                                Red::Handle<Red::AppearanceDefinition>& aAppearance)
{
#if defined(__APPLE__)
    Mac::Count(s_hits[HitRegisterPart]);
#endif
#ifndef NDEBUG
    LogDebug("[{}] [event=RegisterPart part={}]", ExtensionName, aPart->path.hash);
#endif

    std::unique_lock _(s_mutex);
    UpdatePartAssignments(aComponentStorage->components, aPart->path);
}

#if defined(__APPLE__)
Raw::GarmentAssembler::GarmentProcessorResult App::GarmentExtension::OnProcessGarment(uintptr_t a2, uintptr_t a3,
                                                                                      Red::GarmentLoadingParams* aParams)
{
    Mac::Count(s_hits[HitProcessGarment]);

    if (!aParams)
        return Raw::GarmentAssembler::ProcessGarment(a2, a3, aParams);

    std::unique_lock _(s_mutex);
    if (auto& entityState = s_stateManager->FindEntityState(aParams->entity))
    {
#ifndef NDEBUG
        LogDebug("[{}] [event=ProcessGarment entity={}]", ExtensionName, entityState->GetName());
#endif

        UpdateDynamicAttributes(entityState);
    }

    auto result = Raw::GarmentAssembler::ProcessGarment(a2, a3, aParams);

    s_stateManager->LinkEntityToAssembler(aParams->entity, Raw::GarmentAssembler::GetProcessor(result));

    return result;
}
#else
uintptr_t App::GarmentExtension::OnProcessGarment(Red::SharedPtr<Red::GarmentProcessingContext>& aProcessor, uintptr_t a2,
                                                       uintptr_t a3, Red::GarmentLoadingParams* aParams)
{
    if (!aParams)
        return Raw::GarmentAssembler::ProcessGarment(aProcessor, a2, a3, aParams);

    std::unique_lock _(s_mutex);
    if (auto& entityState = s_stateManager->FindEntityState(aParams->entity))
    {
#ifndef NDEBUG
        LogDebug("[{}] [event=ProcessGarment entity={}]", ExtensionName, entityState->GetName());
#endif

        UpdateDynamicAttributes(entityState);
    }

    auto result = Raw::GarmentAssembler::ProcessGarment(aProcessor, a2, a3, aParams);

    s_stateManager->LinkEntityToAssembler(aParams->entity, aProcessor);

    return result;
}
#endif

void App::GarmentExtension::OnProcessGarmentMesh(Raw::GarmentAssembler::ProcessMesh aCallback,
                                                 Red::GarmentProcessingContext* aProcessor, uint32_t aIndex,
                                                 const Red::Handle<Red::EntityTemplate>& aPartTemplate,
                                                 const Red::SharedPtr<Red::ResourceToken<>>& aMeshToken,
                                                 const Red::Handle<Red::IComponent>& aComponent,
                                                 const Red::JobGroup& aJobGroup)
{
#if defined(__APPLE__)
    Mac::Count(s_hits[HitProcessMesh]);
#endif
    std::unique_lock _(s_mutex);
    if (auto& entityState = s_stateManager->FindEntityState(aProcessor))
    {
        if (entityState->ApplyDynamicAppearance(aComponent, aPartTemplate->path))
        {
#ifndef NDEBUG
            LogDebug("[{}] [event=ProcessGarmentMesh entity={} comp={}]", ExtensionName, entityState->GetName(), aComponent->name.ToString());
#endif
            // if (aMeshToken->IsFailed())
            // {
                auto meshToken = ComponentWrapper(aComponent).LoadResourceToken(false);
                if (meshToken->IsFinished())
                {
                    aCallback(aProcessor, aIndex, aPartTemplate, meshToken, aComponent, aJobGroup);
                }
                else
                {
                    Red::JobQueue jobQueue{aJobGroup};
                    jobQueue.Wait(meshToken->job);
                    jobQueue.Dispatch([aCallback, aProcessor, aIndex, aPartTemplate, meshToken, aComponent](const Red::JobGroup& aJobGroup) {
                        aCallback(aProcessor, aIndex, aPartTemplate, meshToken, aComponent, aJobGroup);
                    });
                }
                return;
            // }
        }
    }

    aCallback(aProcessor, aIndex, aPartTemplate, aMeshToken, aComponent, aJobGroup);
}

// int32_t App::GarmentOverrideExtension::OnGetBaseMeshOffset(Red::Handle<Red::IComponent>& aComponent,
//                                                         Red::Handle<Red::EntityTemplate>& aTemplate)
// {
//     auto offset = Raw::AppearanceChanger::GetBaseMeshOffset(aComponent, aTemplate);
//
//     if (s_garmentOffsetsEnabled)
//     {
//         if (offset >= -1000)
//         {
//             offset %= 1000;
//
// #ifndef NDEBUG
//             LogDebug("[{}] [event=GetBaseMeshOffset comp={} offset={}]", ExtensionName, aComponent->name.ToString(), offset);
// #endif
//         }
//     }
//
//     return offset;
// }

void App::GarmentExtension::OnComputeGarment(uintptr_t, Red::Handle<Red::Entity>& aEntity,
                                              Red::DynArray<int32_t>& aOffsets,
                                              Red::SharedPtr<Red::GarmentProcessingContext>& aData,
                                              uintptr_t, uintptr_t, uintptr_t, bool)
{
#if defined(__APPLE__)
    Mac::Count(s_hits[HitComputeGarment]);
#endif
    std::unique_lock _(s_mutex);
    if (auto& entityState = s_stateManager->FindEntityState(aEntity))
    {
#ifndef NDEBUG
        LogDebug("[{}] [event=ComputeGarment entity={}]",
                 ExtensionName, entityState->GetName());
#endif

        UpdatePartAssignments(entityState, aData->components, aData->resources);
        UpdateDynamicAttributes(entityState);

        ApplyDynamicAppearance(entityState, aData->components);
        ApplyComponentOverrides(entityState, aData->components, false);
        ApplyOffsetOverrides(entityState, aOffsets, aData->resources);

        PatchHeadGarmentOverrides(entityState, aData->definition);
    }
}

void App::GarmentExtension::OnReassembleAppearance(Red::Entity* aEntity, uintptr_t, uintptr_t, uintptr_t,
                                                        uintptr_t, uintptr_t)
{
#if defined(__APPLE__)
    Mac::Count(s_hits[HitReassemble]);
#endif
    std::unique_lock _(s_mutex);
    if (auto& entityState = s_stateManager->FindEntityState(aEntity))
    {
#ifndef NDEBUG
        LogDebug("[{}] [event=ReassembleAppearance entity={}]",
                 ExtensionName, entityState->GetName());
#endif

        UpdateDynamicAttributes(entityState);

        ApplyDynamicAppearance(entityState);
        ApplyComponentOverrides(entityState, true);
    }
}

#if defined(__APPLE__)
void App::GarmentExtension::OnGameDetach(uintptr_t, uintptr_t)
#else
void App::GarmentExtension::OnGameDetach()
#endif
{
#if defined(__APPLE__)
    Mac::Count(s_hits[HitGameDetach]);
#endif

    std::unique_lock _(s_mutex);
    s_stateManager->ClearStates();
}

void App::GarmentExtension::RegisterOffsetOverrides(Core::SharedPtr<EntityState>& aEntityState, uint64_t aHash,
                                                         Red::Handle<Red::AppearanceDefinition>& aApperance,
                                                         int32_t aOffset)
{
    for (const auto& partValue : aApperance->partsValues)
    {
        aEntityState->AddOffsetOverride(aHash, partValue.resource.path, aOffset);
    }
}

void App::GarmentExtension::RegisterComponentOverrides(Core::SharedPtr<EntityState>& aEntityState, uint64_t aHash,
                                                       Red::Handle<Red::AppearanceDefinition>& aApperance)
{
    RegisterComponentOverrides(aEntityState, aHash, aApperance->partsOverrides);

    for (const auto& visualTag : aApperance->visualTags.tags)
    {
        for (auto& [componentName, chunkMask] : s_tagManager->GetOverrides(visualTag))
        {
            aEntityState->AddChunkMaskOverride(aHash, componentName, chunkMask.mask, chunkMask.set);
        }
    }
}

void App::GarmentExtension::RegisterComponentOverrides(Core::SharedPtr<EntityState>& aEntityState, uint64_t aHash,
                                                       Red::DynArray<Red::AppearancePartOverrides>& aOverrides)
{
    for (const auto& partOverrides : aOverrides)
    {
        if (!partOverrides.partResource.path)
        {
            for (const auto& componentOverride : partOverrides.componentsOverrides)
            {
                aEntityState->AddChunkMaskOverride(aHash, componentOverride.componentName, componentOverride.chunkMask);
                aEntityState->AddAppearanceOverride(aHash, componentOverride.componentName, componentOverride.meshAppearance);
            }
        }
    }
}

void App::GarmentExtension::UnregisterOffsetOverrides(Core::SharedPtr<EntityState>& aEntityState, uint64_t aHash)
{
    aEntityState->RemoveOffsetOverrides(aHash);
}

void App::GarmentExtension::UnregisterComponentOverrides(Core::SharedPtr<EntityState>& aEntityState, uint64_t aHash)
{
    aEntityState->RemoveChunkMaskOverrides(aHash);
    aEntityState->RemoveAppearanceOverrides(aHash);
}

void App::GarmentExtension::UpdatePartAttributes(Core::SharedPtr<EntityState>& aEntityState,
                                                      Red::Handle<Red::AppearanceDefinition>& aApperance)
{
    for (const auto& partValue : aApperance->partsValues)
    {
        aEntityState->LinkPartToAppearance(partValue.resource.path, aApperance->name);
    }
}

void App::GarmentExtension::UpdatePartAssignments(Red::DynArray<Red::Handle<Red::IComponent>>& aComponents,
                                                       Red::ResourcePath aPartResource)
{
    for (auto i = 0; i < aComponents.Size(); ++i)
    {
        s_stateManager->LinkComponentToPart(aComponents[i], aPartResource);
    }
}

void App::GarmentExtension::UpdatePartAssignments(Core::SharedPtr<App::EntityState>& aEntityState,
                                                       Red::DynArray<Red::Handle<Red::IComponent>>& aComponents,
                                                       Red::DynArray<Red::ResourcePath>& aPartResources)
{
    for (auto i = 0; i < aComponents.Size(); ++i)
    {
        aEntityState->LinkComponentToPart(aComponents[i], aPartResources[i]);
    }
}

bool App::GarmentExtension::PrepareDynamicAppearanceName(Red::WeakHandle<Red::Entity>& aEntity,
                                                              Red::ResourceTokenPtr<Red::EntityTemplate>& aTemplateToken,
                                                              Red::CName& aAppearanceName)
{
    if (IsUniqueAppearanceName(aAppearanceName) &&
        s_dynamicAppearance->SupportsDynamicAppearance(aTemplateToken->resource) &&
        s_dynamicAppearance->IsDynamicAppearanceName(aAppearanceName))
    {
        s_dynamicAppearance->MarkDynamicAppearanceName(aAppearanceName, aEntity.instance);
        return true;
    }

    return false;
}

void App::GarmentExtension::SelectDynamicAppearance(Core::SharedPtr<App::EntityState>& aEntityState,
                                                        App::DynamicAppearanceName& aSelector,
                                                        Red::EntityTemplate* aResource,
                                                        Red::TemplateAppearance*& aAppearance)
{
    aEntityState->SelectDynamicAppearance(aSelector, aResource, aAppearance);
}

void App::GarmentExtension::SelectDynamicAppearance(Core::SharedPtr<EntityState>& aEntityState,
                                                         DynamicAppearanceName& aSelector,
                                                         Red::AppearanceResource* aResource,
                                                         Red::Handle<Red::AppearanceDefinition>& aDefinition)
{
    aEntityState->SelectDynamicAppearance(aSelector, aResource, aDefinition);
}

void App::GarmentExtension::ApplyDynamicAppearance(Core::SharedPtr<EntityState>& aEntityState,
                                                        Red::DynArray<Red::Handle<Red::IComponent>>& aComponents)
{
    aEntityState->ToggleConditionalComponents(aComponents);

    for (auto& component : aComponents)
    {
        aEntityState->ApplyDynamicAppearance(component);
    }
}

void App::GarmentExtension::ApplyDynamicAppearance(Core::SharedPtr<EntityState>& aEntityState)
{
    ApplyDynamicAppearance(aEntityState, aEntityState->GetEntity()->components);
}

void App::GarmentExtension::ApplyComponentOverrides(Core::SharedPtr<EntityState>& aEntityState, bool aForceUpdate)
{
    ApplyComponentOverrides(aEntityState, aEntityState->GetEntity()->components, aForceUpdate);
}

void App::GarmentExtension::ApplyComponentOverrides(Core::SharedPtr<EntityState>& aEntityState,
                                                         Red::DynArray<Red::Handle<Red::IComponent>>& aComponents,
                                                         bool aForceUpdate)
{
    for (auto& component : aComponents)
    {
        auto isAppearnceUpdated = aEntityState->ApplyAppearanceOverride(component);
        auto isChunkMaskUpdated = aEntityState->ApplyChunkMaskOverride(component);

        if (aForceUpdate && (isAppearnceUpdated || isChunkMaskUpdated))
        {
            ComponentWrapper wrapper{component};
            if (wrapper.IsMeshComponent() /*&& !wrapper.IsGarmentComponent()*/)
            {
                Raw::IComponent::UpdateRenderer(component);
            }
        }
    }

#ifndef NDEBUG
    if (!aForceUpdate)
    {
        auto index = 0;
        for (auto& component : aComponents)
        {
            auto wrapper = ComponentWrapper(component);

            if (wrapper.IsMeshComponent())
            {
                LogDebug("[{}] [index={} component={} type={} enabled={} app={} chunks={:064b}]",
                         ExtensionName,
                         index,
                         component->name.ToString(),
                         component->GetType()->GetName().ToString(),
                         component->isEnabled,
                         wrapper.GetAppearanceName().ToString(),
                         wrapper.GetChunkMask());
            }
            else
            {
                LogDebug("[{}] [index={} component={} type={} enabled={}]",
                         ExtensionName,
                         index,
                         component->name.ToString(),
                         component->GetType()->GetName().ToString(),
                         component->isEnabled);
            }

            ++index;
        }
    }
#endif
}

void App::GarmentExtension::ApplyOffsetOverrides(Core::SharedPtr<App::EntityState>& aEntityState,
                                                      Red::DynArray<int32_t>& aOffsets,
                                                      Red::DynArray<Red::ResourcePath>& aResourcePaths)
{
    if (aOffsets.Size() != aResourcePaths.Size())
    {
        aOffsets.Reserve(aResourcePaths.Size());
    }

    if (s_garmentOffsetsEnabled)
    {
        aEntityState->ApplyOffsetOverrides(aResourcePaths, aOffsets);
    }
    else
    {
        aOffsets.Clear();
    }
}

void App::GarmentExtension::EnableGarmentOffsets()
{
    s_garmentOffsetsEnabled = true;
}

void App::GarmentExtension::DisableGarmentOffsets()
{
    s_garmentOffsetsEnabled = false;
}

void App::GarmentExtension::UpdateDynamicAttributes(Core::SharedPtr<EntityState>& aEntityState)
{
    aEntityState->UpdateDynamicAttributes();
}

void App::GarmentExtension::UpdateDynamicAttributes(Core::SharedPtr<EntityState>& aEntityState,
                                                         Red::TweakDBID aEquippedItemID)
{
    aEntityState->UpdateDynamicAttributes(aEquippedItemID);
}

void App::GarmentExtension::UpdateDynamicAttributes()
{
    s_stateManager->UpdateDynamicAttributes();
}

bool App::GarmentExtension::IsUniqueAppearanceName(Red::CName aName)
{
    return aName && aName != DefaultAppearanceName && aName != RandomAppearanceName && aName != EmptyAppearanceName;
}

void App::GarmentExtension::PatchHeadGarmentOverrides(Core::SharedPtr<EntityState>& aEntityState,
                                                           Red::Handle<Red::AppearanceDefinition>& aDefinition)
{
    if (aDefinition->name != AggregatedHeadAppearanceName)
        return;

    for (const auto& part : aDefinition->partsValues)
    {
        for (const auto& componentName : aEntityState->GetPartComponentNames(part.resource.path))
        {
            bool hasComponentOverride = false;

            for (auto& partOverride : aDefinition->partsOverrides)
            {
                if (partOverride.partResource.path
                    && partOverride.partResource.path != part.resource.path
                    && partOverride.partResource.path != DummyAppearancePartPath)
                    continue;

                for (auto& componentOverride : partOverride.componentsOverrides)
                {
                    if (componentOverride.componentName == componentName)
                    {
                        hasComponentOverride = true;
                        break;
                    }
                }

                if (hasComponentOverride)
                {
                    break;
                }
            }

            if (!hasComponentOverride)
            {
                Red::appearancePartComponentOverrides componentOverride;
                componentOverride.componentName = componentName;
                componentOverride.meshAppearance = DefaultAppearanceName;
                componentOverride.chunkMask = std::numeric_limits<uint64_t>::max();
                componentOverride.acceptDismemberment = true;

                Red::appearanceAppearancePartOverrides partOverride;
                partOverride.partResource = DummyAppearancePartPath;
                partOverride.componentsOverrides.PushBack(std::move(componentOverride));

                aDefinition->partsOverrides.PushBack(std::move(partOverride));
            }
        }
    }
}

Core::SharedPtr<App::DynamicAppearanceController>& App::GarmentExtension::GetDynamicAppearanceController()
{
    return s_dynamicAppearance;
}

Core::SharedPtr<App::OverrideTagManager>& App::GarmentExtension::GetTagManager()
{
    return s_tagManager;
}
