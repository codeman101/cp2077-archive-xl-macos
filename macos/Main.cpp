#include "MacCrashDiag.hpp"
#include "MacExit.hpp"
#include "MacDiag.hpp"
#include "MacV1Natives.hpp"

#include "App/Application.hpp"
#include "App/Project.hpp"
#include "Core/Facades/Runtime.hpp"

#include <cstdio>
#include <exception>

namespace
{
bool g_initialized = false;

// The application: runtime paths, the address resolver, the hooking driver, logging and the services.
Core::UniquePtr<App::Application> g_app;

// An address inside this dylib. Core::ModuleImage turns it into the plugin path with dladdr.
int g_moduleAnchor = 0;
}

// RED4ext (macOS port: the plugin API without the v1 namespace)

RED4EXT_C_EXPORT bool RED4EXT_CALL Main(RED4ext::v1::PluginHandle aHandle, RED4ext::v1::EMainReason aReason,
                                        const RED4ext::v1::Sdk* aSdk)
{
    MAC_DIAG("[axl] Main(%d)\n", static_cast<int>(aReason));
    try
    {
        switch (aReason)
        {
        case RED4ext::v1::EMainReason::Load:
        {
            if (g_initialized)
            {
                return true;
            }
            g_initialized = true;
            Mac::MarkPluginLoaded(aHandle, aSdk, "AXL");
            Mac::InstallCrashDiag();

            g_app = Core::MakeUnique<App::Application>(&g_moduleAnchor, aHandle, aSdk);
            g_app->Bootstrap();
#if defined(CP2077_MAC_V1)
            Mac::V1::AddNativeRegistrationCallback("axl");
#endif
            MAC_DIAG("[axl] Main load root=%s module=%s\n", Core::Runtime::GetRootDir().string().c_str(),
                     Core::Runtime::GetModuleDir().string().c_str());
            break;
        }
        case RED4ext::v1::EMainReason::Unload:
        {
            g_initialized = false;
            if (g_app)
            {
                g_app->Shutdown();
                // R27: the plugins are unloaded from RED4ext's atexit handler, so the process is already leaving. The application owns tokens, handles and hook
                // state of the game; releasing them here goes through SDK relocations and the game's allocators while the game shuts down (quit crash in
                // ~SharedPtr -> UniversalRelocBase::Resolve). The hooks are off after Shutdown(); the memory is left to the process exit.
                (void)g_app.release();
            }
            MAC_DIAG("[axl] Main unload\n");
            Mac::ExitWhenAllPluginsUnloaded("AXL");
            break;
        }
        }
        return true;
    }
    catch (const std::exception& ex)
    {
        std::fprintf(stderr, "[axl] Main exception %s\n", ex.what());
        return false;
    }
    catch (...)
    {
        std::fprintf(stderr, "[axl] Main exception\n");
        return false;
    }
}

RED4EXT_C_EXPORT void RED4EXT_CALL Query(RED4ext::v1::PluginInfo* aInfo)
{
    aInfo->name = App::Project::NameW;
    aInfo->author = App::Project::AuthorW;
    aInfo->version = RED4EXT_V1_SEMVER(App::Project::Version.major, App::Project::Version.minor, App::Project::Version.patch);
    aInfo->runtime = RED4EXT_V1_RUNTIME_VERSION_INDEPENDENT;
    aInfo->sdk = RED4EXT_V1_SDK_VERSION_1_0_0_COMPAT_0_5_0;
}

RED4EXT_C_EXPORT uint32_t RED4EXT_CALL Supports()
{
    return RED4EXT_API_VERSION_1_COMPAT_0;
}
