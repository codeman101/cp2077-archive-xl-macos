#include "ArchiveService.hpp"
#include "Core/Facades/Runtime.hpp"
#if defined(__APPLE__)
#include "MacDepotDiag.hpp"
#include "App/Utils/MacProbe.hpp"
#endif

App::ArchiveService::ArchiveService(std::filesystem::path aGameDir, std::filesystem::path aBundleDir)
    : m_gameDir(std::move(aGameDir))
    , m_bundleDir(std::move(aBundleDir))
    , m_loaded(false)
{
#if defined(__APPLE__)
    LoadSkipList();

    // macOS (R22). The Mac game mounts archive/Mac/{content,ep1,patch} only, not archive/Mac/mod (Windows mounts archive/pc/mod natively).
    // The loose mod archives are mounted here, as a Mod-scope group, which also lets the extension discovery find the .xl files
    // next to them. The bundle is registered only when it exists (the packed ArchiveXL.archive is not shipped on macOS yet).
    std::error_code ec;
    if (!m_bundleDir.empty() && std::filesystem::is_directory(m_bundleDir, ec))
    {
        RegisterDirectory(m_bundleDir);
    }
    const auto modDir = m_gameDir / "archive" / "Mac" / "mod";
    if (App::MacProbe::NoMountRequested())
    {
        std::fprintf(stderr, "[axl] archive/Mac/mod mount disabled by ARCHIVEXL_MAC_NOMOUNT (probe boot)\n");
    }
    else if (std::filesystem::is_directory(modDir, ec))
    {
        RegisterDirectory(modDir);
    }
#else
    if (!m_bundleDir.empty())
    {
        RegisterDirectory(m_bundleDir);
    }
#endif
}

void App::ArchiveService::OnBootstrap()
{
    HookAfter<Raw::ResourceDepot::InitializeArchives>(&ArchiveService::OnInitializeArchives).OrThrow();
}

void App::ArchiveService::OnShutdown()
{
    Unhook<Raw::ResourceDepot::InitializeArchives>();
}

void App::ArchiveService::OnInitializeArchives(Red::ResourceDepot* aDepot)
{
    Red::RegisterPendingTypes();

    m_loaded = true;

#if defined(__APPLE__)
    LogInfo("InitializeArchives returned.");
    Mac::DumpDepot(aDepot, "after InitializeArchives");
#endif

    LogInfo("Loading extra archives...");

    if (m_dirs.empty() && m_archives.empty())
        return;

    Core::Vector<std::filesystem::path> loadedArchives;
    Red::DynArray<Red::ResourcePath> loadedResources;

    for (const auto& archiveDir : m_dirs)
    {
        std::error_code error;
        auto dirIt = std::filesystem::directory_iterator(archiveDir, error);

        if (error)
        {
            LogError("Can't load archive directory \"{}\": {}",
                     std::filesystem::relative(archiveDir, m_gameDir).string(),
                     error.message());
            continue;
        }

        Red::DynArray<Red::CString> archivePaths;

#if defined(__APPLE__)
        // Directory order is the file system's; the depot is searched in a stable order only when the paths are sorted.
        Core::Vector<std::filesystem::path> found;
        for (const auto& entry : dirIt)
        {
            if (entry.is_regular_file() && entry.path().extension() == L".archive")
            {
                if (m_skipNames.contains(entry.path().filename().string()))
                {
                    LogInfo("Archive \"{}\" is not mounted (listed in mac-mount-skip.txt).",
                            std::filesystem::relative(entry.path(), m_gameDir).string());
                    continue;
                }

                found.push_back(entry.path());
            }
        }
        // Byte order (ASCII, case sensitive). The Mac depot takes the first archive of a group that has a resource, so the first file in
        // this order wins. R25: byte order is the game's own rule. The Windows engine lists a group with the "*.archive" finder (0x7bb008)
        // and sorts the names with an insertion sort (0x7bb0bc) whose comparator (0xbe0efc) is the plain byte compare of two CStrings
        // (unsigned, no case folding); it is not the case-insensitive order of the NTFS directory listing. A case-insensitive sort (as in
        // jackmaxwil/cp2077-archive-xl-macos) would order "zebra.archive" before "_x.archive" (upper-cased 'Z' is below '_'), the game the other way round, so it is not used.
        std::sort(found.begin(), found.end());
        for (const auto& path : found)
        {
            archivePaths.PushBack(path.string());

            if (archiveDir != m_bundleDir)
            {
                loadedArchives.push_back(path);
            }
        }
#else
        for (const auto& entry : dirIt)
        {
            if (entry.is_regular_file() && entry.path().extension() == L".archive")
            {
                archivePaths.PushBack(entry.path().string());

                if (archiveDir != m_bundleDir)
                {
                    loadedArchives.push_back(entry.path());
                }
            }
        }
#endif

        auto& group = ResolveArchiveGroup(aDepot, archiveDir.string());

        if (!archivePaths.IsEmpty())
        {
#if defined(__APPLE__)
            Raw::ResourceDepot::LoadArchives(nullptr, group, archivePaths, loadedResources, false, Raw::ResourceDepot::LoadArchivesMode);
#else
            Raw::ResourceDepot::LoadArchives(nullptr, group, archivePaths, loadedResources, false);
#endif
        }
    }

    if (!m_archives.empty())
    {
        Red::DynArray<Red::CString> archivePaths;

        for (const auto& archivePath : m_archives)
        {
            archivePaths.PushBack(archivePath.string());
            loadedArchives.push_back(archivePath);
        }

        auto& group = ResolveArchiveGroup(aDepot, "");
#if defined(__APPLE__)
        Raw::ResourceDepot::LoadArchives(nullptr, group, archivePaths, loadedResources, false, Raw::ResourceDepot::LoadArchivesMode);
#else
        Raw::ResourceDepot::LoadArchives(nullptr, group, archivePaths, loadedResources, false);
#endif
    }

    for (const auto& archivePath : loadedArchives)
    {
        LogInfo("Archive \"{}\" loaded.", std::filesystem::relative(archivePath, m_gameDir).string());
    }

#if defined(__APPLE__)
    Mac::DumpDepot(aDepot, "after extra archives");
#endif
}

Red::ArchiveGroup& App::ArchiveService::ResolveArchiveGroup(Red::ResourceDepot* aDepot, const Red::CString& aBasePath)
{
    auto existingGroup = std::find_if(aDepot->groups.begin(), aDepot->groups.end(),
                                     [&aBasePath](const Red::ArchiveGroup& aGroup) {
                                         return aGroup.basePath == aBasePath;
                                     });

    if (existingGroup != aDepot->groups.end())
    {
        return *existingGroup;
    }

    auto firstNonModGroup = std::find_if(aDepot->groups.begin(), aDepot->groups.end(),
                                         [](const Red::ArchiveGroup& aGroup) {
                                             return aGroup.scope != Red::ArchiveScope::Mod;
                                         });
    auto firstNonModGroupIndex = firstNonModGroup - aDepot->groups.begin();

    aDepot->groups.Emplace(firstNonModGroup);

    auto& group = aDepot->groups[firstNonModGroupIndex];
    group.basePath = aBasePath;
    group.scope = Red::ArchiveScope::Mod;

    return group;
}

bool App::ArchiveService::RegisterArchive(std::filesystem::path aPath)
{
    std::error_code error;

    if (aPath.is_relative())
    {
        aPath = m_gameDir / aPath;
    }

    if (!std::filesystem::exists(aPath, error) || !std::filesystem::is_regular_file(aPath, error))
    {
        LogError("Can't register archive \"{}\": path doesn't exist.",
                 std::filesystem::relative(aPath, m_gameDir).string());
        return false;
    }

    if (m_loaded)
    {
        LogError("Can't register archive \"{}\": depot is already initialized.",
                 std::filesystem::relative(aPath, m_gameDir).string());
        return false;
    }

    m_archives.emplace_back(std::move(aPath));
    return true;
}

bool App::ArchiveService::RegisterDirectory(std::filesystem::path aPath)
{
    std::error_code error;

    if (aPath.is_relative())
    {
        aPath = m_gameDir / aPath;
    }

    if (!std::filesystem::exists(aPath, error) || !std::filesystem::is_directory(aPath, error))
    {
        LogError("Can't register archive directory \"{}\": path doesn't exist.",
                 std::filesystem::relative(aPath, m_gameDir).string());
        return false;
    }

    if (m_loaded)
    {
        LogError("Can't register archive directory \"{}\": depot is already initialized.",
                 std::filesystem::relative(aPath, m_gameDir).string());
        return false;
    }

    m_dirs.emplace_back(std::move(aPath));
    return true;
}

#if defined(__APPLE__)
void App::ArchiveService::LoadSkipList()
{
    // mac-mount-skip.txt next to the plugin: one archive file name per line, '#' starts a comment. Archives that the CP2077 Mac Archive Patcher
    // already baked into the official archives belong here, so that the loose copies are not mounted a second time.
    m_skipNames.clear();

    const auto path = Core::Runtime::GetModuleDir() / "mac-mount-skip.txt";
    std::ifstream file(path);
    if (!file)
        return;

    std::string line;
    while (std::getline(file, line))
    {
        if (const auto comment = line.find('#'); comment != std::string::npos && (comment == 0 || line[comment - 1] == ' ' || line[comment - 1] == '\t'))
            line.erase(comment);

        const auto first = line.find_first_not_of(" \t\r\n");
        if (first == std::string::npos)
            continue;
        const auto last = line.find_last_not_of(" \t\r\n");
        m_skipNames.insert(line.substr(first, last - first + 1));
    }

    if (!m_skipNames.empty())
        LogInfo("mac-mount-skip.txt: {} archive name(s) are not mounted.", m_skipNames.size());
}
#endif
