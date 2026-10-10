#include "ResourcePathRegistry.hpp"
#include "App/Project.hpp"
#include "Red/SharedStorage.hpp"

namespace
{
constexpr auto SharedName = Red::CName("ResourcePathRegistryV3" BUILD_SUFFIX);
}

App::ResourcePathRegistry::ResourcePathRegistry(const std::filesystem::path& aPreloadPath)
{
    s_instance = Red::AcquireSharedInstance<SharedName, SharedInstance>();
    s_preloadPath = aPreloadPath;
}

void App::ResourcePathRegistry::OnBootstrap()
{
    std::unique_lock lock(s_instance->m_lock);

    if (!s_instance->m_initialized)
    {
        s_instance->m_initialized = true;
        s_instance->m_map.reserve(400000);

        // If another plugin (Codeware) published the registry first, m_initialized is already true and its hook fills the shared map.
        HookAfter<Raw::ResourcePath::Create>(&OnCreatePath);
    }

    if (!s_instance->m_preloaded && !s_preloadPath.empty() && std::filesystem::exists(s_preloadPath))
    {
        s_instance->m_preloaded = true;

        std::thread([lock = std::move(lock)]() {
            LogInfo("[ResourcePathRegistry] Loading metadata...");

            std::ifstream f(s_preloadPath);
            std::string s;
            while (std::getline(f, s))
            {
                s_instance->m_map[Red::ResourcePath::HashSanitized(s.data())] = std::move(s);
            }

            LogInfo("[ResourcePathRegistry] Loaded {} predefined hashes.", s_instance->m_map.size());
        }).detach();
    }
}

#if defined(__APPLE__)
void App::ResourcePathRegistry::OnCreatePath(uint64_t& aHash, const char* aData, uint32_t aSize)
{
    if (aData && aSize > 0 && aHash)
    {
        s_instance->m_hits.fetch_add(1, std::memory_order_relaxed);
        std::scoped_lock _(s_instance->m_lock);
        s_instance->m_map[Red::ResourcePath(aHash)] = {aData, aSize};
    }
}
#else
void App::ResourcePathRegistry::OnCreatePath(Red::ResourcePath* aPath, Red::StringView* aPathStr)
{
    if (aPathStr && *aPathStr)
    {
        std::scoped_lock _(s_instance->m_lock);
        s_instance->m_map[*aPath] = {aPathStr->Data(), aPathStr->Length()};
    }
}
#endif

std::string App::ResourcePathRegistry::ResolvePath(Red::ResourcePath aPath)
{
    if (!aPath)
        return {};

    std::shared_lock _(s_instance->m_lock);
    const auto& it = s_instance->m_map.find(aPath);

    if (it == s_instance->m_map.end())
        return {};

    return it.value();
}

std::string App::ResourcePathRegistry::ResolvePathOrHash(Red::ResourcePath aPath)
{
    auto str = ResolvePath(aPath);

    if (str.empty())
    {
        str = std::to_string(aPath.hash);
    }

    return str;
}

Red::ResourcePath App::ResourcePathRegistry::RegisterPath(const std::string& aPathStr)
{
    if (aPathStr.empty())
        return {};

    auto path = Red::ResourcePath(aPathStr.data());

    RegisterPath(path, aPathStr);

    return path;
}

void App::ResourcePathRegistry::RegisterPath(Red::ResourcePath aPath, const std::string& aPathStr)
{
    if (!aPath)
        return;

    {
        std::shared_lock _(s_instance->m_lock);
        if (s_instance->m_map.contains(aPath))
            return;
    }

    {
        std::scoped_lock _(s_instance->m_lock);
        s_instance->m_map[aPath] = aPathStr;
    }
}
