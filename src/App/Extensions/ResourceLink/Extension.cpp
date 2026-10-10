#include "Extension.hpp"
#include <cstring>
#include "App/Extensions/ResourceMeta/Extension.hpp"
#include "App/Shared/ResourcePathRegistry.hpp"
#include "Core/Facades/Container.hpp"
#if defined(__APPLE__)
#include "MacDiag.hpp"
#endif

namespace
{
constexpr auto ExtensionName = "ResourceManager";
}

std::string_view App::ResourceLinkExtension::GetName()
{
    return ExtensionName;
}

bool App::ResourceLinkExtension::Load()
{
    HookBefore<Raw::ResourceLoader::LoadAsync>(&OnLoaderResourceRequest).OrThrow();
#if defined(__APPLE__)
    HookWrap<Raw::ResourceLoader::LoadAsyncByPath>(&OnLoaderPathRequest).OrThrow();
#endif
    Hook<Raw::ResourceDepot::RequestResource>(&OnDepotResourceRequest).OrThrow();
    Hook<Raw::ResourceDepot::CheckResource>(&OnDepotResourceCheck).OrThrow();

    return true;
}

bool App::ResourceLinkExtension::Unload()
{
    Unhook<Raw::ResourceLoader::LoadAsync>();
#if defined(__APPLE__)
    Unhook<Raw::ResourceLoader::LoadAsyncByPath>();
#endif
    Unhook<Raw::ResourceDepot::RequestResource>();
    Unhook<Raw::ResourceDepot::CheckResource>();

    return true;
}

void App::ResourceLinkExtension::Configure()
{
    std::scoped_lock _(s_linksLock, s_copiesLock, s_mappingsLock);

    s_mappings.clear();
    s_copies.clear();
    s_links.clear();

    auto depot = Red::ResourceDepot::Get();

    Core::Map<Red::ResourcePath, Red::ResourcePath> finalCopies;
    Core::Map<Red::ResourcePath, Red::ResourcePath> finalLinks;
    Core::Map<Red::ResourcePath, std::string> knownPaths;
    Core::Set<Red::ResourcePath> invalidPaths;

    for (auto& config : m_configs)
    {
        for (const auto& [targetPath, copyPaths] : config.copies)
        {
            for (const auto& copyPath : ResourceMetaExtension::ExpandList(copyPaths))
            {
                if (copyPath == targetPath)
                {
                    if (!invalidPaths.contains(copyPath))
                    {
                        LogError("[{}] Copy \"{}\" points to itself.", ExtensionName, config.paths[copyPath]);
                        invalidPaths.insert(copyPath);
                    }
                    continue;
                }

                if (Raw::ResourceDepot::CheckResource(depot, copyPath))
                {
                    if (!invalidPaths.contains(copyPath))
                    {
                        LogError("[{}] Copy \"{}\" is an existing resource.", ExtensionName, config.paths[copyPath]);
                        invalidPaths.insert(copyPath);
                    }
                    continue;
                }

                finalCopies[copyPath] = targetPath;
                knownPaths[copyPath] = config.paths[copyPath];
            }

            knownPaths[targetPath] = config.paths[targetPath];
        }
    }

    for (auto& config : m_configs)
    {
        for (const auto& [targetPath, linkPaths] : config.links)
        {
            for (const auto& linkPath : ResourceMetaExtension::ExpandList(linkPaths))
            {
                if (linkPath == targetPath)
                {
                    if (!invalidPaths.contains(linkPath))
                    {
                        LogError("[{}] Link \"{}\" points to itself.", ExtensionName, config.paths[linkPath]);
                        invalidPaths.insert(linkPath);
                    }
                    continue;
                }

                if (Raw::ResourceDepot::CheckResource(depot, linkPath) || finalCopies.contains(linkPath))
                {
                    if (!invalidPaths.contains(linkPath))
                    {
                        LogError("[{}] Link \"{}\" is an existing resource.", ExtensionName, config.paths[linkPath]);
                        invalidPaths.insert(linkPath);
                    }
                    continue;
                }

                // TODO: Warn about redefinitions?

                finalLinks[linkPath] = targetPath;
                knownPaths[linkPath] = config.paths[linkPath];
            }

            knownPaths[targetPath] = config.paths[targetPath];
        }
    }

    Core::Set<Red::ResourcePath> cyclicLinks;

    for (auto link = finalLinks.begin(); link != finalLinks.end();)
    {
        const auto& linkPath = link->first;
        auto targetPath = link->second;
        auto isCyclic = false;

        Core::Set<Red::ResourcePath> hops;
        hops.insert(linkPath);
        hops.insert(targetPath);

        while (true)
        {
            const auto& next = finalLinks.find(targetPath);

            if (next == finalLinks.end())
                break;

            targetPath = next->second;

            if (hops.contains(targetPath))
            {
                isCyclic = true;
                break;
            }

            hops.insert(targetPath);
        }

        if (isCyclic)
        {
            cyclicLinks.insert(linkPath);
        }
        else
        {
            link.value() = targetPath;
        }

        ++link;
    }

    for (const auto& linkPath : cyclicLinks)
    {
        if (!invalidPaths.contains(linkPath))
        {
            LogError("[{}] Link \"{}\" is cyclic.", ExtensionName, knownPaths[linkPath]);
            invalidPaths.insert(linkPath);
        }

        finalLinks.erase(linkPath);
    }

    for (auto link = finalLinks.begin(); link != finalLinks.end();)
    {
        const auto& linkPath = link->first;
        const auto& targetPath = link->second;

        if (!Raw::ResourceDepot::CheckResource(depot, targetPath) && !finalCopies.contains(targetPath))
        {
            if (!invalidPaths.contains(targetPath))
            {
                LogError(R"([{}] Link "{}" points to a non-existent resource "{}".)",
                         ExtensionName, knownPaths[linkPath], knownPaths[targetPath]);
                invalidPaths.insert(targetPath);
            }

            link = finalLinks.erase(link);
            continue;
        }

        ++link;
    }

    auto resourcePathRegistry = Core::Resolve<ResourcePathRegistry>();
    for (const auto& [knownPath, knownPathStr] : knownPaths)
    {
        resourcePathRegistry->RegisterPath(knownPath, knownPathStr);
    }

    s_copies = std::move(finalCopies);
    s_links = std::move(finalLinks);

    s_mappings.insert(s_copies.begin(), s_copies.end());
    s_mappings.insert(s_links.begin(), s_links.end());

    m_configs.clear();
}

#if defined(__APPLE__)
void App::ResourceLinkExtension::OnLoaderResourceRequest(Red::ResourceLoader* aLoader, Red::ResourceRequest& aRequest)
#else
void App::ResourceLinkExtension::OnLoaderResourceRequest(Red::ResourceLoader* aLoader,
                                                         Red::SharedPtr<Red::ResourceToken<>>& aToken,
                                                         Red::ResourceRequest& aRequest)
#endif
{
    std::shared_lock _(s_linksLock);
    const auto& link = s_links.find(aRequest.path);
    if (link != s_links.end())
    {
#if defined(__APPLE__)
        Mac::Count(s_loaderHits);
#endif
        aRequest.path = link->second;
    }
}

#if defined(__APPLE__)
// Mac (R22): a second entry for the same redirect. The ByPath function inlines the request function, so the hook on that one never sees
// the path loads (most LoadAsync(path) and RaRef loads). The handler is a full wrapper because a by-value argument cannot be changed by an observer.
Red::SharedPtr<Red::ResourceToken<>> App::ResourceLinkExtension::OnLoaderPathRequest(
    Core::Detail::OriginalFunc<Red::SharedPtr<Red::ResourceToken<>>, Red::ResourceLoader*, Red::ResourcePath> aOriginal,
    Red::ResourceLoader* aLoader, Red::ResourcePath aPath)
{
    {
        std::shared_lock _(s_linksLock);
        const auto& link = s_links.find(aPath);
        if (link != s_links.end())
        {
            Mac::Count(s_pathHits);
            aPath = link->second;
        }
    }

    return aOriginal(aLoader, aPath);
}

Red::DepotResourceHandle App::ResourceLinkExtension::OnDepotResourceRequest(Red::ResourceDepot* aDepot,
                                                                            Red::ResourcePath aPath,
                                                                            const int32_t* aArchiveHandle)
#else
uintptr_t* App::ResourceLinkExtension::OnDepotResourceRequest(Red::ResourceDepot* aDepot,
                                                              const uintptr_t* aResourceHandle,
                                                              Red::ResourcePath aPath,
                                                              const int32_t* aArchiveHandle)
#endif
{
    {
        std::shared_lock _(s_copiesLock);
        const auto& link = s_copies.find(aPath);
        if (link != s_copies.end())
        {
#if defined(__APPLE__)
            Mac::Count(s_copyHits);
#endif
            aPath = link->second;
        }
    }

#if defined(__APPLE__)
    return Raw::ResourceDepot::RequestResource(aDepot, aPath, aArchiveHandle);
#else
    return Raw::ResourceDepot::RequestResource(aDepot, aResourceHandle, aPath, aArchiveHandle);
#endif
}

bool App::ResourceLinkExtension::OnDepotResourceCheck(Red::ResourceDepot* aDepot, Red::ResourcePath aPath)
{
    {
        std::shared_lock _(s_mappingsLock);
        const auto& link = s_mappings.find(aPath);
        if (link != s_mappings.end())
        {
#if defined(__APPLE__)
            Mac::Count(s_checkHits);
#endif
            aPath = link->second;
        }
    }

    return Raw::ResourceDepot::CheckResource(aDepot, aPath);
}

#if defined(__APPLE__)
namespace
{
// Hashes the game loads at the menu (Codeware R13 audio diagnostics): base\sound\event\eventsmetadata.json, base\sound\... volanguagedatamap.json.
constexpr uint64_t SelfTestPaths[] = {0xd7b908a4d0e79997ull, 0x9324cfa977b76a28ull};
bool s_selfTestArmed = false;
}

void App::ResourceLinkExtension::ArmMacSelfTest()
{
    if (!Mac::DiagEnabled() || s_selfTestArmed)
        return;

    s_selfTestArmed = true;

    for (const auto hash : SelfTestPaths)
    {
        const Red::ResourcePath path(hash);
        {
            std::unique_lock _(s_copiesLock);
            s_copies.emplace(path, path);
        }
        {
            std::unique_lock _(s_linksLock);
            s_links.emplace(path, path);
        }
        {
            std::unique_lock _(s_mappingsLock);
            s_mappings.emplace(path, path);
        }
    }

    MAC_DIAG("[selftest] R22 ResourceLink armed: %zu identity mappings (copy, link, check) on resources the game loads later\n",
             sizeof(SelfTestPaths) / sizeof(SelfTestPaths[0]));
}

void App::ResourceLinkExtension::LogMacStats(const char* aWhere)
{
    size_t copies = 0;
    size_t links = 0;
    size_t mappings = 0;
    {
        std::shared_lock _(s_copiesLock);
        copies = s_copies.size();
    }
    {
        std::shared_lock _(s_linksLock);
        links = s_links.size();
    }
    {
        std::shared_lock _(s_mappingsLock);
        mappings = s_mappings.size();
    }

    // The test mappings are identity and are removed after the t+ reading.
    const bool disarm = s_selfTestArmed && std::strncmp(aWhere, "t+", 2) == 0;

    MAC_DIAG("[selftest] R22 ResourceLink %s: %zu copies, %zu links, %zu mappings; remapped so far: loader request %u, loader by path %u, depot request %u, depot check %u\n",
             aWhere, copies, links, mappings, s_loaderHits.load(), s_pathHits.load(), s_copyHits.load(), s_checkHits.load());

    if (disarm)
    {
        s_selfTestArmed = false;
        for (const auto hash : SelfTestPaths)
        {
            const Red::ResourcePath path(hash);
            {
                std::unique_lock _(s_copiesLock);
                s_copies.erase(path);
            }
            {
                std::unique_lock _(s_linksLock);
                s_links.erase(path);
            }
            {
                std::unique_lock _(s_mappingsLock);
                s_mappings.erase(path);
            }
        }
    }
}
#endif

Core::Set<Red::ResourcePath> App::ResourceLinkExtension::GetAliases(Red::ResourcePath aPath)
{
    Core::Set<Red::ResourcePath> result;

    {
        std::shared_lock _(s_mappingsLock);
        for (const auto& [from, to] : s_mappings)
        {
            if (from == aPath)
            {
                result.insert(to);
            }
            else if (to == aPath)
            {
                result.insert(from);
            }
        }
    }

    return result;
}

void App::ResourceLinkExtension::RegisterLink(Red::ResourcePath aLink, Red::ResourcePath aTarget)
{
    std::scoped_lock _(s_linksLock, s_mappingsLock);
    s_links[aLink] = aTarget;
    s_mappings[aLink] = aTarget;
}
