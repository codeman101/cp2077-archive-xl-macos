#pragma once

#include <atomic>

#include "App/Extensions/ExtensionBase.hpp"
#include "App/Extensions/Animation/Config.hpp"
#include "App/Shared/ResourcePathRegistry.hpp"

namespace App
{
class AnimationExtension : public ConfigurableExtensionImpl<AnimationsConfig>
{
public:
    std::string_view GetName() override;
    bool Load() override;
    bool Unload() override;
    void Configure() override;

private:
    static void OnInitializeAnimations(Red::entAnimatedComponent* aComponent);

#if defined(__APPLE__)
    // R22 (CODEWARE_MAC_DIAG=1, read-only): the configured targets and how often the hook ran / found a setup.
    static void LogMacStats(const char* aWhere);
    inline static std::atomic<uint32_t> s_hookHits{0};
    inline static std::atomic<uint32_t> s_matchHits{0};
#endif

    inline static Core::Map<uint64_t, Core::Vector<Red::animAnimSetupEntry>> s_animsByTarget;
    inline static Core::SharedPtr<ResourcePathRegistry> s_resourcePathRegistry;
};
}
