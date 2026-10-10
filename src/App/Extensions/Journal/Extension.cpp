#include "Extension.hpp"
#include "App/Utils/Registers.hpp"
#include "Red/JobHandle.hpp"
#include "Red/Localization.hpp"
#include "Red/WorldNode.hpp"

namespace
{
constexpr auto ExtensionName = "Journal";

constexpr auto PathSeparator = '/';
constexpr auto EditMarker = '*';
constexpr auto IdentityProp = Red::CName("id");

const Red::ClassLocator<Red::gameJournalQuestMapPin> s_questMappinType;
const Red::ClassLocator<Red::gameJournalQuestMapPinBase> s_questMappinBaseType;
const Red::ClassLocator<Red::gameJournalPointOfInterestMappin> s_pointOfInterestType;
const Red::TypeLocator<"LocalizationString"> s_localizationStringType;
}

std::string_view App::JournalExtension::GetName()
{
    return ExtensionName;
}

bool App::JournalExtension::Load()
{
    HookBefore<Raw::JournalTree::ProcessJournalIndex>(&JournalExtension::OnLoadJournal).OrThrow();
    HookBefore<Raw::JournalRootFolderEntry::Initialize>(&JournalExtension::OnInitializeRoot).OrThrow();
    HookAfter<Raw::MappinSystem::OnStreamingWorldLoaded>(&JournalExtension::OnMappinDataLoaded).OrThrow();
#if defined(__APPLE__)
    Hook<Raw::MappinSystem::FindMappinData>(&JournalExtension::OnFindMappinData).OrThrow();
    Hook<Raw::MappinSystem::FindPoiData>(&JournalExtension::OnFindPoiData).OrThrow();
#else
    Hook<Raw::MappinSystem::GetMappinData>(&JournalExtension::OnGetMappinData).OrThrow();
    Hook<Raw::MappinSystem::GetPoiData>(&JournalExtension::OnGetPoiData).OrThrow();
#endif

    return true;
}

bool App::JournalExtension::Unload()
{
    Unhook<Raw::JournalTree::ProcessJournalIndex>();
    Unhook<Raw::JournalRootFolderEntry::Initialize>();
    Unhook<Raw::MappinSystem::OnStreamingWorldLoaded>();
#if defined(__APPLE__)
    Unhook<Raw::MappinSystem::FindMappinData>();
    Unhook<Raw::MappinSystem::FindPoiData>();
#else
    Unhook<Raw::MappinSystem::GetMappinData>();
    Unhook<Raw::MappinSystem::GetPoiData>();
#endif

    return true;
}

void App::JournalExtension::Reload()
{
    if (!m_configs.empty())
    {
        ResetResourceData();
        ResetRuntimeData();
        ReloadJournal();
    }
}

void App::JournalExtension::OnLoadJournal(uintptr_t a1, Red::JobGroup& aJobGroup)
{
    LogInfo("[{}] Journal tree is initializing...", ExtensionName);

    if (m_configs.empty())
    {
        LogInfo("[{}] No entries to merge.", ExtensionName);
        return;
    }

    auto queue = Red::JobQueue(aJobGroup);
    auto loader = Red::ResourceLoader::Get();

    for (const auto& unit : m_configs)
    {
        // LogInfo("[{}] Processing \"{}\"...", ExtensionName, unit.name);

        for (const auto& path : unit.journals)
        {
            auto token = loader->LoadAsync<Red::game::JournalResource>(path.c_str());

            if (!token->IsLoaded())
            {
                queue.Wait(token->job);
            }

            s_resources.emplace_back(token);
            s_paths.emplace(token->path, path);
        }
    }
}

void App::JournalExtension::OnInitializeRoot(Red::game::JournalRootFolderEntry* aJournalRoot, uintptr_t, uintptr_t,
                                             Red::JobQueue& aJobQueue)
{
    static const auto s_rootEntryType = Red::GetClass<Red::game::JournalRootFolderEntry>();

    if (m_configs.empty())
        return;

    auto successAll = true;
    auto mergedAny = false;

    for (const auto& resource : s_resources)
    {
        if (resource->IsFailed())
        {
            LogError("[{}] Resource \"{}\" failed to load.", ExtensionName, s_paths[resource->path]);
            successAll = false;
            continue;
        }

        auto root = resource->Get()->entry.GetPtr<Red::game::JournalContainerEntry>();

        if (!root->GetType()->IsA(s_rootEntryType))
        {
            LogError("[{}] Resource \"{}\" root entry must be {}.",
                     ExtensionName, s_paths[resource->path], s_rootEntryType->GetName().ToString());
            successAll = false;
            continue;
        }

        if (root->id != aJournalRoot->id)
            continue;

        LogInfo("[{}] Merging entries from \"{}\"...", ExtensionName, s_paths[resource->path]);

        successAll &= MergeEntries(aJournalRoot, root, root->id.c_str());
        mergedAny = true;
    }

    if (mergedAny)
    {
        if (successAll)
            LogInfo("[{}] All journal entries merged.", ExtensionName);
        else
            LogWarning("[{}] Journal entries merged with issues.", ExtensionName);
    }
}

void App::JournalExtension::OnMappinDataLoaded(void* aMappinSystem, Red::worldRuntimeScene*)
{
    auto cookedMappinResource = Raw::MappinSystem::CookedMappinResource::Ptr(aMappinSystem)->instance;
    if (!cookedMappinResource->cookedData.IsEmpty() && cookedMappinResource->cookedData.Size() == cookedMappinResource->cookedData.Capacity())
    {
        {
            const auto reserve = std::max<size_t>(s_mappins.size() << 1, cookedMappinResource->cookedData.Size() / 2ull);
            cookedMappinResource->cookedData.Reserve(cookedMappinResource->cookedData.Size() + reserve);
        }
        {
            const auto reserve = std::max<size_t>(s_mappins.size() << 1, cookedMappinResource->cookedMultiData.Size() / 2ull);
            cookedMappinResource->cookedMultiData.Reserve(cookedMappinResource->cookedMultiData.Size() + reserve);
        }
    }

    auto cookedPoiResource = Raw::MappinSystem::CookedPoiResource::Ptr(aMappinSystem)->instance;
    if (!cookedPoiResource->cookedData.IsEmpty() && cookedPoiResource->cookedData.Size() == cookedPoiResource->cookedData.Capacity())
    {
        const auto reserve = std::max<size_t>(s_mappins.size() << 1, cookedPoiResource->cookedData.Size() / 2ull);
        cookedPoiResource->cookedData.Reserve(cookedPoiResource->cookedData.Size() + reserve);
    }
}

#if defined(__APPLE__)
// macOS (R22). The cooked resource arrives as the first argument; a journal mappin is resolved by the finder of its own kind.
void* App::JournalExtension::OnFindMappinData(void* aResource, uint32_t aHash)
{
    auto result = Raw::MappinSystem::FindMappinData(aResource, aHash);

    if (!result && aResource && !s_mappins.empty())
    {
        const auto it = s_mappins.find(aHash);
        if (it != s_mappins.end() && !it.value().isPointOfInterest)
        {
            ResolveCookedMappin(aResource, aHash, it.value(), result);
        }
    }

    return result;
}

void* App::JournalExtension::OnFindPoiData(void* aResource, uint32_t aHash)
{
    auto result = Raw::MappinSystem::FindPoiData(aResource, aHash);

    if (!result && aResource && !s_mappins.empty())
    {
        const auto it = s_mappins.find(aHash);
        if (it != s_mappins.end() && it.value().isPointOfInterest)
        {
            ResolveCookedMappin(aResource, aHash, it.value(), result);
        }
    }

    return result;
}

#endif

void* App::JournalExtension::OnGetMappinData(void* aMappinSystem, uint32_t aHash)
{
    auto result = Raw::MappinSystem::GetMappinData(aMappinSystem, aHash);

    if (!result && !s_mappins.empty())
    {
        auto r8 = get_r8();
        auto r9 = get_r9();

        const auto it = s_mappins.find(aHash);
        if (it != s_mappins.end())
        {
            ResolveCookedMappin(aMappinSystem, aHash, it.value(), result);
        }

        set_r8(r8);
        set_r9(r9);
    }

    return result;
}

void* App::JournalExtension::OnGetPoiData(void* aMappinSystem, uint32_t aHash)
{
    auto result = Raw::MappinSystem::GetPoiData(aMappinSystem, aHash);

    if (!result && !s_mappins.empty())
    {
        auto r8 = get_r8();
        auto r9 = get_r9();

        auto it = s_mappins.find(aHash);
        if (it != s_mappins.end())
        {
            ResolveCookedMappin(aMappinSystem, aHash, it.value(), result);
        }

        set_r8(r8);
        set_r9(r9);
    }

    return result;
}

void App::JournalExtension::ResolveCookedMappin(void* aMappinSystem, uint32_t aHash,
                                                JournalMappin& aJournalMappin, void*& aCookedMappin)
{
    ResolveMappinReference(aJournalMappin);

    if (aJournalMappin.isPointOfInterest)
    {
        Red::gameCookedPointOfInterestMappinData cookedMappin{};

        if (ResolveMappinPosition(aHash, aJournalMappin, cookedMappin.position))
        {
            cookedMappin.journalPathHash = aHash;
            cookedMappin.entityID.hash = aJournalMappin.reference.hash;

            std::unique_lock _(s_mappinsLock);
#if defined(__APPLE__)
            auto resource = static_cast<Red::gamePointOfInterestMappinResource*>(aMappinSystem); // the finder's resource argument
#else
            auto resource = Raw::MappinSystem::CookedPoiResource::Ptr(aMappinSystem)->instance;
#endif
            resource->cookedData.PushBack(std::move(cookedMappin));

            aCookedMappin = &resource->cookedData.Back();
        }
    }
    else
    {
        Red::gameCookedMappinData cookedMappin{};

        if (ResolveMappinPosition(aHash, aJournalMappin, cookedMappin.position))
        {
            ResolveMappinVolume(aHash, aJournalMappin, cookedMappin.volume);

            cookedMappin.journalPathHash = aHash;

            std::unique_lock _(s_mappinsLock);
#if defined(__APPLE__)
            auto resource = static_cast<Red::gameMappinResource*>(aMappinSystem); // the finder's resource argument
#else
            auto resource = Raw::MappinSystem::CookedMappinResource::Ptr(aMappinSystem)->instance;
#endif
            resource->cookedData.PushBack(std::move(cookedMappin));

            aCookedMappin = &resource->cookedData.Back();
        }
    }
}

void App::JournalExtension::ResolveMappinReference(JournalMappin& aMappin)
{
    if (aMappin.reference.hash && !aMappin.resolved.hash)
    {
        Red::worldGlobalNodeRef context{Red::NodeRef::GlobalRoot};
        Red::CallGlobal("ResolveNodeRef", aMappin.resolved, aMappin.reference, context);
    }
}

bool App::JournalExtension::ResolveMappinPosition(uint32_t aHash, const JournalMappin& aMappin, Red::Vector3& aResult)
{
    LogInfo("[{}] Cooked mappin #{} ({}) requested...", ExtensionName, aHash, aMappin.path);

    if (!aMappin.reference.hash)
    {
        aResult = aMappin.offset;

        LogInfo("[{}] Cooked mappin #{} ({}) resolved to static offset.", ExtensionName, aHash, aMappin.path);

        return true;
    }

    if (!aMappin.resolved.hash)
    {
        LogError("[{}] Can't resolve mappin #{} ({}) reference.", ExtensionName, aHash, aMappin.path);
        return false;
    }

    bool success{};
    Red::Transform transform{};
#if defined(__APPLE__)
    // R27 (Mac): the default ScriptGameInstance takes the instance from CGameEngine::Get(), whose data address has no Mac entry (it reads as zeros), so
    // the instance stayed null and GetNodeTransform failed for every cooked mappin ("Can't resolve mappin #... position", found by the autotest harness
    // for Lizzie's bar pin). The live instance comes from Codeware's hooks, the same one its GetGameInstance replacement hands to scripts.
    Red::ScriptGameInstance game{Red::MacGameInstance()};
#else
    Red::ScriptGameInstance game{};
#endif
    Red::CallStatic("ScriptGameInstance", "GetNodeTransform", success, game, aMappin.resolved, transform);

    if (!success)
    {
        LogError("[{}] Can't resolve mappin #{} ({}) position.", ExtensionName, aHash, aMappin.path);
        return false;
    }

    aResult.X = transform.position.X + aMappin.offset.X;
    aResult.Y = transform.position.Y + aMappin.offset.Y;
    aResult.Z = transform.position.Z + aMappin.offset.Z;

    LogInfo("[{}] Cooked mappin #{} ({}) resolved to NodeRef #{}.",
            ExtensionName, aHash, aMappin.path, aMappin.resolved.hash);

    return true;
}

bool App::JournalExtension::ResolveMappinVolume(uint32_t aJournalHash,
                                                const App::JournalExtension::JournalMappin& aMappin,
                                                Red::Handle<Red::gamemappinsIMappinVolume>& aResult)
{
    if (!aMappin.resolved.hash)
        return false;

    auto nativeNodeRegistry = Red::GetRuntimeSystem<Red::worldNodeInstanceRegistry>();

#if defined(__APPLE__)
    auto nodeInstance = Raw::WorldNodeRegistry::FindNode(nativeNodeRegistry, aMappin.resolved.hash);
#else
    Red::Handle<Red::worldINodeInstance> nodeInstance;
    Raw::WorldNodeRegistry::FindNode(nativeNodeRegistry, nodeInstance, aMappin.resolved.hash);
#endif

    if (!nodeInstance)
        return false;

    const auto areaNode = Red::Cast<Red::worldAreaShapeNode>(Raw::WorldNodeInstance::Node::Ref(nodeInstance));

    if (!areaNode || !areaNode->outline)
        return false;

    const auto& scale = Raw::WorldNodeInstance::Scale::Ref(nodeInstance);
    const auto& transform = Raw::WorldNodeInstance::Transform::Ref(nodeInstance);

    const auto volume = Red::MakeHandle<Red::gamemappinsOutlineMappinVolume>();
    volume->height = areaNode->outline->height  * scale.Z;
    volume->outlinePoints.Reserve(areaNode->outline->points.Size());

    for (const auto& areaPoint : areaNode->outline->points)
    {
        auto mappinPoint = Red::Vector4(areaPoint) * scale * transform.orientation;
        volume->outlinePoints.EmplaceBack(mappinPoint.X, mappinPoint.Y);
    }

    aResult = volume;

    return true;
}

App::JournalExtension::EntrySearchResult App::JournalExtension::FindEntry(Red::game::JournalEntry* aParent,
                                                                          Red::CString& aPath)
{
    static const auto s_containerEntryType = Red::GetClass<Red::game::JournalContainerEntry>();

    if (!aParent)
        return {};

    Red::game::JournalContainerEntry* parentEntry = nullptr;
    Red::game::JournalEntry* finalEntry = aParent;
    std::string finalPath;
    bool markedForEdit = false;

    std::stringstream ss(aPath.c_str());
    std::string id;

    while (std::getline(ss, id, PathSeparator))
    {
        if (!finalEntry->GetType()->IsA(s_containerEntryType))
            return {};

        parentEntry = reinterpret_cast<Red::game::JournalContainerEntry*>(finalEntry);

        if (ss.eof() && id.ends_with(EditMarker))
        {
            id.erase(id.end() - 1);
            markedForEdit = true;
        }

        if (!finalPath.empty())
            finalPath += PathSeparator;

        finalPath += id;

        auto it = std::find_if(parentEntry->entries.Begin(), parentEntry->entries.End(),
            [&id](const Red::Handle<Red::game::JournalEntry>& aEntry) -> bool
            {
               return strcmp(aEntry->id.c_str(), id.c_str()) == 0;
            });

        if (it == parentEntry->entries.End())
        {
            return {nullptr, finalPath, ss.eof() ? parentEntry : nullptr, markedForEdit};
        }

        finalEntry = it->GetPtr();
    }

    return {finalEntry, finalPath, parentEntry, markedForEdit};
}

bool App::JournalExtension::MergeEntries(Red::game::JournalContainerEntry* aTarget,
                                         Red::game::JournalContainerEntry* aSource,
                                         const std::string& aPath)
{
    auto success = true;

    for (const auto& sourceEntry : aSource->entries)
    {
        auto [targetEntry, targetId, parentEntry, markedForEdit] = FindEntry(aTarget, sourceEntry->id);
        auto targetPath = MakePath(aPath, targetId);

        if (targetEntry)
        {
            success &= MergeEntry(targetEntry, sourceEntry, targetPath, markedForEdit);
        }
        else if (parentEntry)
        {
            if (markedForEdit)
            {
                LogWarning("[{}] {}: Cannot modify entry, path not fould.", ExtensionName, targetPath);
                success = false;
                continue;
            }

            ProcessNewEntries(sourceEntry, targetPath, true);

            parentEntry->entries.EmplaceBack(sourceEntry);
        }
        else
        {
            LogWarning("[{}] {}: Path not fould.", ExtensionName, targetPath);
            success = false;
        }
    }

    return success;
}

bool App::JournalExtension::MergeEntry(Red::game::JournalEntry* aTarget, Red::game::JournalEntry* aSource,
                                       const std::string& aPath, bool aEditProps)
{
    static const auto s_containerEntryType = Red::GetClass<Red::game::JournalContainerEntry>();

    auto success = true;

    if (aEditProps)
    {
        if (aTarget->GetType() == aSource->GetType())
        {
            ConvertLocKeys(aSource);

            Red::DynArray<Red::CProperty*> props;
            aTarget->GetType()->GetProperties(props);

            for (const auto& prop : props)
            {
                if (prop->name == IdentityProp)
                    continue;

                prop->SetValue(aTarget, prop->GetValuePtr<void>(aSource));
            }
        }
        else
        {
            LogWarning("[{}] {}: Cannot modify entry, type mismatch.", ExtensionName, aPath);
            success = false;
        }
    }

    if (aTarget->GetType()->IsA(s_containerEntryType) && aSource->GetType()->IsA(s_containerEntryType))
    {
        success &= MergeEntries(reinterpret_cast<Red::game::JournalContainerEntry*>(aTarget),
                                reinterpret_cast<Red::game::JournalContainerEntry*>(aSource),
                                aPath);
    }

    return success;
}

void App::JournalExtension::ProcessNewEntries(Red::game::JournalEntry* aEntry, const std::string& aPath, bool aRecursive)
{
    static const auto s_containerEntryType = Red::GetClass<Red::game::JournalContainerEntry>();

    ConvertLocKeys(aEntry);
    CollectMappin(aEntry, aPath);

    if (aRecursive && aEntry->GetType()->IsA(s_containerEntryType))
    {
        for (const auto& entry : reinterpret_cast<Red::game::JournalContainerEntry*>(aEntry)->entries)
        {
            ProcessNewEntries(entry, MakePath(aPath, entry->id.c_str()), true);
        }
    }
}

void App::JournalExtension::ConvertLocKeys(Red::game::JournalEntry* aEntry)
{
    Red::DynArray<Red::CProperty*> props;
    aEntry->GetType()->GetProperties(props);

    for (const auto& prop : props)
    {
        if (prop->type == s_localizationStringType)
        {
            auto value = prop->GetValuePtr<Red::LocalizationString>(aEntry);
            auto lockey = std::string(value->unk08.c_str());

            if (!lockey.starts_with(Red::LocKeyPrefix))
            {
                auto hash = Red::FNV1a64(lockey.c_str());

                lockey.clear();
                lockey.append(Red::LocKeyPrefix);
                lockey.append(std::to_string(hash));

                value->unk08 = lockey.c_str();
            }
        }
    }
}

void App::JournalExtension::CollectMappin(Red::game::JournalEntry* aEntry, const std::string& aPath)
{
    auto entryType = aEntry->GetType();

    if (entryType->IsA(s_questMappinType))
    {
        auto hash = CalculateJournalHash(aPath);
        auto entry = reinterpret_cast<Red::gameJournalQuestMapPin*>(aEntry);

        if (!entry->reference.dynamicEntityUniqueName)
        {
            s_mappins.insert({hash, {aPath, entry->reference.reference, entry->offset, false}});

            if (!entry->reference.reference.hash)
            {
                entry->offset = {};
            }
        }
    }
    else if (entryType->IsA(s_questMappinBaseType))
    {
        auto hash = CalculateJournalHash(aPath);
        auto entry = reinterpret_cast<Red::gameJournalQuestMapPinBase*>(aEntry);

        s_mappins.insert({hash, {aPath, 0ull, entry->offset, false}});
    }
    else if (entryType->IsA(s_pointOfInterestType))
    {
        auto hash = CalculateJournalHash(aPath);
        auto entry = reinterpret_cast<Red::gameJournalPointOfInterestMappin*>(aEntry);

        s_mappins.insert({hash, {aPath, entry->staticNodeRef, entry->offset, true}});

        if (!entry->staticNodeRef.hash)
        {
            entry->offset = {};
        }
    }
}

void App::JournalExtension::ResetResourceData()
{
    s_resources.clear();
    s_paths.clear();
}

void App::JournalExtension::ResetRuntimeData()
{
    s_mappins.clear();
}

void App::JournalExtension::ReloadJournal()
{
    auto manager = Red::GetGameSystem<Red::game::IJournalManager>();

    if (!manager)
        return;

    Red::JournalEntryHash questHash{};
    Red::JournalEntryHash poiHash{};

    {
#if defined(__APPLE__)
        Red::Handle<Red::game::JournalEntry> quest = Raw::JournalManager::GetTrackedQuest(manager);
        Red::Handle<Red::game::JournalEntry> poi = Raw::JournalManager::GetTrackedPointOfInterest(manager);
#else
        Red::Handle<Red::game::JournalEntry> quest;
        Red::Handle<Red::game::JournalEntry> poi;

        Raw::JournalManager::GetTrackedQuest(manager, quest);
        Raw::JournalManager::GetTrackedPointOfInterest(manager, poi);
#endif

        if (quest)
            questHash = Raw::JournalManager::GetEntryHash(manager, quest);

        if (poi)
            poiHash = Raw::JournalManager::GetEntryHash(manager, poi);
    }

    {
        Red::Handle<Red::game::JournalEntry> null;
        Raw::JournalManager::TrackQuest(manager, null);
        Raw::JournalManager::TrackPointOfInterest(manager, null);
    }

#if defined(__APPLE__)
    Red::JobHandle job = Raw::JournalManager::LoadJournal(manager);
#else
    Red::JobHandle job{};
    Raw::JournalManager::LoadJournal(manager, job);
#endif
    Raw::JobHandle::Wait(job);

    if (questHash)
    {
#if defined(__APPLE__)
        Red::Handle<Red::game::JournalEntry> quest = Red::Cast<Red::game::JournalEntry>(Raw::JournalManager::GetEntryByHash(manager, questHash));
#else
        Red::Handle<Red::game::JournalEntry> quest;
        Raw::JournalManager::GetEntryByHash(manager, quest, questHash);
#endif

        if (quest)
            Raw::JournalManager::TrackQuest(manager, quest);
    }

    if (poiHash)
    {
#if defined(__APPLE__)
        Red::Handle<Red::game::JournalEntry> poi = Red::Cast<Red::game::JournalEntry>(Raw::JournalManager::GetEntryByHash(manager, poiHash));
#else
        Red::Handle<Red::game::JournalEntry> poi;
        Raw::JournalManager::GetEntryByHash(manager, poi, poiHash);
#endif

        if (poiHash)
            Raw::JournalManager::TrackPointOfInterest(manager, poi);
    }
}

std::string App::JournalExtension::MakePath(const std::string& aPath, const std::string& aStep)
{
    return !aPath.empty() ? aPath + PathSeparator + aStep : aStep;
}

uint32_t App::JournalExtension::CalculateJournalHash(const std::string& aPath)
{
    return Red::Murmur3_32(reinterpret_cast<const uint8_t*>(aPath.data()), aPath.length());
}
