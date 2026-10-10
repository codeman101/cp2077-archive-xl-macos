#pragma once

#include <atomic>

#include "App/Extensions/ExtensionBase.hpp"
#include "App/Extensions/ResourceLink/Config.hpp"
#include "Red/ResourceDepot.hpp"
#include "Red/ResourceLoader.hpp"

namespace App
{
class ResourceLinkExtension : public ConfigurableExtensionImpl<ResourceLinkConfig>
{
public:
    std::string_view GetName() override;
    bool Load() override;
    bool Unload() override;
    void Configure() override;

    static Core::Set<Red::ResourcePath> GetAliases(Red::ResourcePath aPath);
    static void RegisterLink(Red::ResourcePath aPath, Red::ResourcePath aLink);

#if defined(__APPLE__)
    // R22 (CODEWARE_MAC_DIAG=1, read-only): the mapping counts and how often each hook remapped a path.
    static void LogMacStats(const char* aWhere);

    // R22 (CODEWARE_MAC_DIAG=1): maps two sound json files the game loads after this call onto themselves (copy, link and check mapping), so the
    // counters above show that the hooks see real game traffic. The mapping changes nothing; LogMacStats("t+...") removes it again.
    static void ArmMacSelfTest();
#endif

private:
#if defined(__APPLE__)
    static void OnLoaderResourceRequest(Red::ResourceLoader* aLoader, Red::ResourceRequest& aRequest);
    static Red::SharedPtr<Red::ResourceToken<>> OnLoaderPathRequest(
        Core::Detail::OriginalFunc<Red::SharedPtr<Red::ResourceToken<>>, Red::ResourceLoader*, Red::ResourcePath> aOriginal,
        Red::ResourceLoader* aLoader, Red::ResourcePath aPath);
    static Red::DepotResourceHandle OnDepotResourceRequest(Red::ResourceDepot* aDepot, Red::ResourcePath aPath,
                                                           const int32_t* aArchiveHandle);
#else
    static void OnLoaderResourceRequest(Red::ResourceLoader* aLoader, Red::SharedPtr<Red::ResourceToken<>>& aToken,
                                        Red::ResourceRequest& aRequest);
    static uintptr_t* OnDepotResourceRequest(Red::ResourceDepot* aDepot, const uintptr_t* aResourceHandle,
                                             Red::ResourcePath aPath, const int32_t* aArchiveHandle);
#endif
    static bool OnDepotResourceCheck(Red::ResourceDepot* aDepot, Red::ResourcePath aPath);

#if defined(__APPLE__)
    inline static std::atomic<uint32_t> s_loaderHits{0};
    inline static std::atomic<uint32_t> s_pathHits{0};
    inline static std::atomic<uint32_t> s_copyHits{0};
    inline static std::atomic<uint32_t> s_checkHits{0};
#endif
    inline static Core::Map<Red::ResourcePath, Red::ResourcePath> s_mappings;
    inline static Red::SharedSpinLock s_mappingsLock;
    inline static Core::Map<Red::ResourcePath, Red::ResourcePath> s_copies;
    inline static Red::SharedSpinLock s_copiesLock;
    inline static Core::Map<Red::ResourcePath, Red::ResourcePath> s_links;
    inline static Red::SharedSpinLock s_linksLock;
};
}
