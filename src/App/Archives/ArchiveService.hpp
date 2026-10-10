#pragma once

#include "Core/Foundation/Feature.hpp"
#include "Core/Hooking/HookingAgent.hpp"
#include "Core/Logging/LoggingAgent.hpp"
#include "Red/ResourceDepot.hpp"

namespace App
{
class ArchiveService
    : public Core::Feature
    , public Core::HookingAgent
    , public Core::LoggingAgent
{
public:
    explicit ArchiveService(std::filesystem::path aGameDir, std::filesystem::path aBundleDir = {});

    bool RegisterArchive(std::filesystem::path aPath);
    bool RegisterDirectory(std::filesystem::path aPath);

protected:
    void OnBootstrap() override;
    void OnShutdown() override;

    void OnInitializeArchives(Red::ResourceDepot* aDepot);
    static Red::ArchiveGroup& ResolveArchiveGroup(Red::ResourceDepot* aDepot, const Red::CString& aBasePath);

#if defined(__APPLE__)
    // macOS (R22): archive file names that are not mounted (mac-mount-skip.txt next to the plugin, one name per line).
    void LoadSkipList();
    Core::Set<std::string> m_skipNames;
#endif

    Core::Vector<std::filesystem::path> m_archives;
    Core::Vector<std::filesystem::path> m_dirs;
    std::filesystem::path m_bundleDir;
    std::filesystem::path m_gameDir;
    bool m_loaded;
};
}
