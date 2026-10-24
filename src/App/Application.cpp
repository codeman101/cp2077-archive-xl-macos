#include "Application.hpp"
#include "App/Archives/ArchiveService.hpp"
#include "App/Environment.hpp"
#include "App/Extensions/ExtensionService.hpp"
#include "App/Migration.hpp"
#include "App/Patches/EntitySpawnerPatch.hpp"
#include "App/Patches/WorldWidgetLimitPatch.hpp"
#include "App/Project.hpp"
#include "App/Shared/ResourcePathRegistry.hpp"
#include "Core/Foundation/RuntimeProvider.hpp"
#include "Support/RedLib/RedLibProvider.hpp"

#if defined(__APPLE__)
#include "MacProviders.hpp"
#else
#include "Support/MinHook/MinHookProvider.hpp"
#include "Support/RED4ext/RED4extProvider.hpp"
#include "Support/Spdlog/SpdlogProvider.hpp"
#endif

#if defined(__APPLE__)
App::Application::Application(void* aModule, RED4ext::v1::PluginHandle aPlugin, const RED4ext::v1::Sdk* aSdk)
{
    // MAC-NOT-YET: no SpdlogProvider (log lines go to stderr with the prefix "[axl]"; the launcher captures them), no
    // RED4extProvider::RegisterScripts (the script cache is built offline by tools/build-cache.py) and no Migration::CleanUp
    // (it would delete the legacy r6/scripts/ArchiveXL folder).
    // Depth 3: <game>/Cyberpunk2077.app/Contents/MacOS/Cyberpunk2077 -> the game folder.
    Register<Core::RuntimeProvider>(aModule)
        ->SetBaseImagePathDepth(3);

    // Address resolver, hooking driver and logging (stand-in for MinHookProvider, SpdlogProvider and RED4extProvider).
    Register<Support::MacPlatformProvider>(aPlugin, aSdk);
    Register<Support::RedLibProvider>();

    Register<App::ResourcePathRegistry>();
    Register<App::ArchiveService>(Env::GameDir(), Env::BundleDir());
    Register<App::ExtensionService>(Env::BundleDir());
    Register<App::EntitySpawnerPatch>();
    Register<App::WorldWidgetLimitPatch>();
}
#else
App::Application::Application(HMODULE aHandle, const RED4ext::v1::Sdk* aSdk)
{
    Register<Core::RuntimeProvider>(aHandle)
        ->SetBaseImagePathDepth(2);

    Register<Support::MinHookProvider>();
    Register<Support::SpdlogProvider>()
        ->AppendTimestampToLogName()
        ->CreateRecentLogSymlink()
        ->SetMaxLogFiles(5);
    Register<Support::RED4extProvider>(aHandle, aSdk)
        ->EnableAddressLibrary()
        ->RegisterScripts(Env::ScriptsDir());
    Register<Support::RedLibProvider>();

    Register<App::ResourcePathRegistry>();
    Register<App::ArchiveService>(Env::GameDir(), Env::BundleDir());
    Register<App::ExtensionService>(Env::BundleDir());
    Register<App::EntitySpawnerPatch>();
    Register<App::WorldWidgetLimitPatch>();
}
#endif

void App::Application::OnStarting()
{
    LogInfo("{} {} is starting...", Project::Name, Project::Version.to_string());

#if !defined(__APPLE__)
    Migration::CleanUp(Env::LegacyBundleDir());
    Migration::CleanUp(Env::LegacyScriptsDir());
#endif
}
