#if defined(__APPLE__)
// macOS only: the diagnostic probe of the depot and the extension statistics (runs with CODEWARE_MAC_DIAG=1).
#include "MacProbe.hpp"
#include "Core/Facades/Runtime.hpp"
#include "Red/ResourceDepot.hpp"
#include "App/Extensions/ResourceLink/Extension.hpp"
#include "App/Extensions/Mesh/Extension.hpp"
#include "App/Facade.hpp"
#include "MacDepotDiag.hpp"
#include "MacDiag.hpp"
#include "MacAddressResolver.hpp"
#include "Core/Hooking/HookHits.hpp"

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <thread>
#include <vector>

namespace
{
// The first file hash of a Red Archive (RDAR): header 0x28 bytes, index at the position in the header, then 28 bytes of table header and
// 56 byte file entries (hash first).
bool FirstArchiveHash(const std::filesystem::path& aPath, uint64_t& aHash)
{
    std::ifstream file(aPath, std::ios::binary);
    if (!file)
        return false;

    char header[40];
    file.read(header, sizeof(header));
    if (!file || std::memcmp(header, "RDAR", 4) != 0)
        return false;

    uint64_t indexPos;
    std::memcpy(&indexPos, header + 8, sizeof(indexPos));

    char table[36];
    file.seekg(static_cast<std::streamoff>(indexPos + 28));
    file.read(table, sizeof(table));
    if (!file)
        return false;

    std::memcpy(&aHash, table, sizeof(aHash));
    return aHash != 0;
}
}

bool App::MacProbe::NoMountRequested()
{
    const char* value = std::getenv("ARCHIVEXL_MAC_NOMOUNT");
    return value && value[0] == '1';
}

namespace
{
std::vector<void (*)(const char*)>& StatsFunctions()
{
    static std::vector<void (*)(const char*)> functions;
    return functions;
}
}

void App::MacProbe::AddStats(void (*aFunction)(const char* aWhere))
{
    StatsFunctions().push_back(aFunction);
}

void App::MacProbe::Run(const char* aWhere)
{
    if (!Mac::DiagEnabled())
        return;

    auto* depot = Red::ResourceDepot::Get();
    if (!depot)
    {
        std::fprintf(stderr, "[axl] probe (%s) no depot\n", aWhere);
        return;
    }

    static const char* kPaths[] = {
        "base\\quest\\cyberpunk2077.quest",                                 // game archive: expected 1
        "mod\\arman3_lizzies_bds\\journal\\lizzies_bds.journal",            // LizziesBDs.archive only
        "mod\\arman3_lizzies_bds\\quest\\lizzies_bds_launcher_3_00.questphase",
    };
    for (const char* path : kPaths)
    {
        Red::ResourcePath resourcePath(path);
        std::fprintf(stderr, "[axl] probe (%s) CheckResource(%s) -> %d\n", aWhere, path,
                     Raw::ResourceDepot::CheckResource(depot, resourcePath) ? 1 : 0);
    }

    // The first resource of each loose archive, by hash. The weather archive overrides a base game resource, so its hash is 1 with or
    // without the mount; the others are new resources.
    const auto modDir = Core::Runtime::GetRootDir() / "archive" / "Mac" / "mod";
    std::error_code ec;
    for (const auto& entry : std::filesystem::directory_iterator(modDir, ec))
    {
        if (entry.path().extension() != ".archive")
            continue;

        uint64_t hash = 0;
        if (FirstArchiveHash(entry.path(), hash))
        {
            std::fprintf(stderr, "[axl] probe (%s) CheckResource(first hash %016llx of %s) -> %d\n", aWhere,
                         static_cast<unsigned long long>(hash), entry.path().filename().string().c_str(),
                         Raw::ResourceDepot::CheckResource(depot, Red::ResourcePath(hash)) ? 1 : 0);
        }
    }

    Mac::DumpDepot(depot, aWhere);
    App::ResourceLinkExtension::LogMacStats(aWhere);

    // The production call of the ArchiveXL.Reload() script native (diagnostics only, ARCHIVEXL_MAC_RELOAD_TEST=1): the next loader update runs the reload.
    if (std::strcmp(aWhere, "t+50s") == 0)
    {
        Core::HookHits::Dump("axl", aWhere, Red::MacGameBase()); // R26: the call count of every attached hook
        const char* reload = std::getenv("ARCHIVEXL_MAC_RELOAD_TEST");
        if (reload && reload[0] == '1')
        {
            std::fprintf(stderr, "[selftest] R22 hot reload: calling Facade::Reload (ARCHIVEXL_MAC_RELOAD_TEST=1)\n");
            App::Facade::Reload();
        }
    }
    App::MeshExtension::LogMacStats(aWhere);
    for (const auto function : StatsFunctions())
        function(aWhere);
    if (std::strcmp(aWhere, "postLoadScripts") == 0)
        App::ResourceLinkExtension::ArmMacSelfTest();
}

void App::MacProbe::RunLater(unsigned aSeconds)
{
    if (!Mac::DiagEnabled())
        return;

    std::thread([aSeconds] {
        std::this_thread::sleep_for(std::chrono::seconds(aSeconds));
        char where[32];
        std::snprintf(where, sizeof(where), "t+%us", aSeconds);
        Run(where);
    }).detach();
}
#endif // __APPLE__
