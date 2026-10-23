#include "Extension.hpp"
#include "Red/GameEngine.hpp"
#if defined(__APPLE__)
#include "MacDiag.hpp"
#endif
#if defined(__APPLE__)
// A Handle<WidgetLibraryItemInstance> returned by value destroys the instance type, which needs its member types complete.
#include <RED4ext/Scripting/Natives/Generated/ink/ActionName.hpp>
#include <RED4ext/Scripting/Natives/Generated/ink/CharacterEvent.hpp>
#include <RED4ext/Scripting/Natives/Generated/ink/CompoundWidget.hpp>
#include <RED4ext/Scripting/Natives/Generated/ink/ControllerProcessor.hpp>
#include <RED4ext/Scripting/Natives/Generated/ink/HUDLayer.hpp>
#include <RED4ext/Scripting/Natives/Generated/ink/HudWidgetSpawnEntry.hpp>
#include <RED4ext/Scripting/Natives/Generated/ink/IEffect.hpp>
#include <RED4ext/Scripting/Natives/Generated/ink/ISystemRequestsHandler.hpp>
#include <RED4ext/Scripting/Natives/Generated/ink/IWidgetComponentWrapper.hpp>
#include <RED4ext/Scripting/Natives/Generated/ink/ImageWidget.hpp>
#include <RED4ext/Scripting/Natives/Generated/ink/InputEvent.hpp>
#include <RED4ext/Scripting/Natives/Generated/ink/Layer.hpp>
#include <RED4ext/Scripting/Natives/Generated/ink/LayerProxy.hpp>
#include <RED4ext/Scripting/Natives/Generated/ink/MultiChildren.hpp>
#include <RED4ext/Scripting/Natives/Generated/ink/PropertyManager.hpp>
#include <RED4ext/Scripting/Natives/Generated/ink/SpawningProcessor.hpp>
#include <RED4ext/Scripting/Natives/Generated/ink/StyleResourceWrapper.hpp>
#include <RED4ext/Scripting/Natives/Generated/ink/UserData.hpp>
#include <RED4ext/Scripting/Natives/Generated/ink/VirtualWindow.hpp>
#include <RED4ext/Scripting/Natives/Generated/ink/WidgetLibraryResourceWrapper.hpp>
#include <RED4ext/Scripting/Natives/Generated/ink/WidgetReference.hpp>
#include <RED4ext/Scripting/Natives/Generated/ink/WorldLayer.hpp>
#include <RED4ext/Scripting/Natives/Generated/ink/WorldWidgetInfos.hpp>
#include <RED4ext/Scripting/Natives/Generated/ink/anim/Processor.hpp>
#endif

namespace
{
constexpr auto ExtensionName = "InkSpawner";

constexpr auto ControllerSeparator = ':';

Red::ClassLocator<Red::ink::IWidgetController> s_gameControllerType;
Red::ClassLocator<Red::ink::WidgetLogicController> s_logicControllerType;
}

std::string_view App::InkSpawnerExtension::GetName()
{
    return ExtensionName;
}

bool App::InkSpawnerExtension::Load()
{
    HookOnceAfter<Raw::CBaseEngine::InitEngine>(+[]() {
        if (Red::GetType<"Codeware">())
        {
            bool isNewCodeware{false};
            Red::CString newCodewareVersion{"1.14.0"};
            Red::CallStatic("Codeware", "Require", isNewCodeware, newCodewareVersion);

            if (isNewCodeware)
            {
#if defined(__APPLE__)
                MAC_DIAG("[selftest] R22 InkSpawner: Codeware 1.14.0 or newer is loaded, so it owns the ink spawning and this extension installs no hooks\n");
#endif
                return;
            }
        }

        Hook<Raw::InkWidgetLibrary::SpawnFromLocal>(&OnSpawnLocal).OrThrow();
        Hook<Raw::InkWidgetLibrary::SpawnFromExternal>(&OnSpawnExternal).OrThrow();
        Hook<Raw::InkWidgetLibrary::AsyncSpawnFromLocal>(&OnAsyncSpawnLocal).OrThrow();
        Hook<Raw::InkWidgetLibrary::AsyncSpawnFromExternal>(&OnAsyncSpawnExternal).OrThrow();
        HookBefore<Raw::InkSpawner::FinishAsyncSpawn>(&OnFinishAsyncSpawn).OrThrow();
    });

    return true;
}

bool App::InkSpawnerExtension::Unload()
{
    Unhook<Raw::InkWidgetLibrary::SpawnFromLocal>();
    Unhook<Raw::InkWidgetLibrary::SpawnFromExternal>();
    Unhook<Raw::InkWidgetLibrary::AsyncSpawnFromLocal>();
    Unhook<Raw::InkWidgetLibrary::AsyncSpawnFromExternal>();
    Unhook<Raw::InkSpawner::FinishAsyncSpawn>();

    return true;
}

#if defined(__APPLE__)
Red::Handle<Red::ink::WidgetLibraryItemInstance> App::InkSpawnerExtension::OnSpawnLocal(
    Red::ink::WidgetLibraryResource& aLibrary, Red::CName aItemName)
{
    auto instance = Raw::InkWidgetLibrary::SpawnFromLocal(aLibrary, aItemName);

    if (!instance)
    {
        auto* itemNameStr = aItemName.ToString();
        auto* controllerSep = strchr(itemNameStr, ControllerSeparator);

        if (controllerSep)
        {
            Red::CName itemName(Red::FNV1a64(reinterpret_cast<const uint8_t*>(itemNameStr), controllerSep - itemNameStr));
            instance = Raw::InkWidgetLibrary::SpawnFromLocal(aLibrary, itemName);

            if (instance)
            {
                InjectController(instance, controllerSep + 1);
            }
        }
    }

    return instance;
}

Red::Handle<Red::ink::WidgetLibraryItemInstance> App::InkSpawnerExtension::OnSpawnExternal(
    Red::ink::WidgetLibraryResource& aLibrary, Red::ResourcePath aExternalPath, Red::CName aItemName)
{
    InjectDependency(aLibrary, aExternalPath);

    return Raw::InkWidgetLibrary::SpawnFromExternal(aLibrary, aExternalPath, aItemName);
}

bool App::InkSpawnerExtension::OnAsyncSpawnLocal(Red::ink::WidgetLibraryResource& aLibrary,
                                                 Red::InkSpawningInfo& aSpawningInfo,
                                                 Red::CName aItemName,
                                                 bool aFlag)
{
    auto* itemNameStr = aItemName.ToString();
    auto* controllerSep = strchr(itemNameStr, ControllerSeparator);

    if (controllerSep)
    {
        aItemName = Red::FNV1a64(reinterpret_cast<const uint8_t*>(itemNameStr), controllerSep - itemNameStr);
    }

    return Raw::InkWidgetLibrary::AsyncSpawnFromLocal(aLibrary, aSpawningInfo, aItemName, aFlag);
}

bool App::InkSpawnerExtension::OnAsyncSpawnExternal(Red::ink::WidgetLibraryResource& aLibrary,
                                                    Red::InkSpawningInfo& aSpawningInfo,
                                                    Red::ResourcePath aExternalPath,
                                                    Red::CName aItemName,
                                                    bool aFlag)
{
    InjectDependency(aLibrary, aExternalPath);

    return Raw::InkWidgetLibrary::AsyncSpawnFromExternal(aLibrary, aSpawningInfo, aExternalPath, aItemName, aFlag);
}
#else
uintptr_t App::InkSpawnerExtension::OnSpawnLocal(Red::ink::WidgetLibraryResource& aLibrary,
                                                 Red::Handle<Red::ink::WidgetLibraryItemInstance>& aInstance,
                                                 Red::CName aItemName)
{
    auto result = Raw::InkWidgetLibrary::SpawnFromLocal(aLibrary, aInstance, aItemName);

    if (!aInstance)
    {
        auto* itemNameStr = aItemName.ToString();
        auto* controllerSep = strchr(itemNameStr, ControllerSeparator);

        if (controllerSep)
        {
            Red::CName itemName(Red::FNV1a64(reinterpret_cast<const uint8_t*>(itemNameStr), controllerSep - itemNameStr));
            Raw::InkWidgetLibrary::SpawnFromLocal(aLibrary, aInstance, itemName);

            if (aInstance)
            {
                InjectController(aInstance, controllerSep + 1);
            }
        }
    }

    return result;
}

uintptr_t App::InkSpawnerExtension::OnSpawnExternal(Red::ink::WidgetLibraryResource& aLibrary,
                                                    Red::Handle<Red::ink::WidgetLibraryItemInstance>& aInstance,
                                                    Red::ResourcePath aExternalPath,
                                                    Red::CName aItemName)
{
    InjectDependency(aLibrary, aExternalPath);

    return Raw::InkWidgetLibrary::SpawnFromExternal(aLibrary, aInstance, aExternalPath, aItemName);
}

bool App::InkSpawnerExtension::OnAsyncSpawnLocal(Red::ink::WidgetLibraryResource& aLibrary,
                                                 Red::InkSpawningInfo& aSpawningInfo,
                                                 Red::CName aItemName,
                                                 bool aFlag)
{
    auto* itemNameStr = aItemName.ToString();
    auto* controllerSep = strchr(itemNameStr, ControllerSeparator);

    if (controllerSep)
    {
        aItemName = Red::FNV1a64(reinterpret_cast<const uint8_t*>(itemNameStr), controllerSep - itemNameStr);
    }

    return Raw::InkWidgetLibrary::AsyncSpawnFromLocal(aLibrary, aSpawningInfo, aItemName, aFlag);
}

bool App::InkSpawnerExtension::OnAsyncSpawnExternal(Red::ink::WidgetLibraryResource& aLibrary,
                                                    Red::InkSpawningInfo& aSpawningInfo,
                                                    Red::ResourcePath aExternalPath,
                                                    Red::CName aItemName)
{
    InjectDependency(aLibrary, aExternalPath);

    return Raw::InkWidgetLibrary::AsyncSpawnFromExternal(aLibrary, aSpawningInfo, aExternalPath, aItemName);
}
#endif

void App::InkSpawnerExtension::OnFinishAsyncSpawn(Red::InkSpawningContext& aContext,
                                                  Red::Handle<Red::ink::WidgetLibraryItemInstance>& aInstance)
{
    auto* itemNameStr = aContext.request->itemName.ToString();
    auto* controllerSep = strchr(itemNameStr, ControllerSeparator);

    if (controllerSep)
    {
        InjectController(aInstance, controllerSep + 1);
    }
}

void App::InkSpawnerExtension::InjectDependency(Red::ink::WidgetLibraryResource& aLibrary, Red::ResourcePath aExternalPath)
{
    bool libraryExists = false;

    // Check if the external library is in the list and do nothing if it is
    {
        std::shared_lock _(s_mutex);
        for (const auto& externalLibrary : aLibrary.externalLibraries)
        {
            if (externalLibrary.path == aExternalPath)
            {
                libraryExists = true;
                break;
            }
        }
    }

    // Add the requested library to the list
    if (!libraryExists)
    {
        std::unique_lock _(s_mutex);
        aLibrary.externalLibraries.EmplaceBack(aExternalPath);

        // Load requested library for the spawner
        auto& externalLibrary = aLibrary.externalLibraries.Back();
        externalLibrary.LoadAsync();

        Red::WaitForResource(externalLibrary.token, std::chrono::milliseconds(1000));
    }
}

void App::InkSpawnerExtension::InjectController(Red::Handle<Red::ink::WidgetLibraryItemInstance>& aInstance,
                                                Red::CName aControllerName)
{
    auto* controllerType = Red::CRTTISystem::Get()->GetClass(aControllerName);

    if (controllerType)
    {
        if (controllerType->IsA(s_gameControllerType))
        {
            auto* controllerInstance = reinterpret_cast<Red::ink::IWidgetController*>(controllerType->CreateInstance(true));
            Red::Handle<Red::ink::IWidgetController> controllerHandle(controllerInstance);

            aInstance->gameController.Swap(controllerHandle);

            if (controllerHandle.instance)
            {
                InheritProperties(controllerInstance, controllerHandle.instance);
            }
        }
        else if (controllerType->IsA(s_logicControllerType))
        {
            auto* controllerInstance = reinterpret_cast<Red::ink::WidgetLogicController*>(controllerType->CreateInstance(true));
            Red::Handle<Red::ink::WidgetLogicController> controllerHandle(controllerInstance);

            aInstance->rootWidget->logicController.Swap(controllerHandle);

            if (controllerHandle.instance)
            {
                InheritProperties(controllerInstance, controllerHandle.instance);
            }
        }
    }
}

void App::InkSpawnerExtension::InheritProperties(Red::IScriptable* aTarget, Red::IScriptable* aSource)
{
    auto* sourceType = aSource->GetType();
    auto* targetType = aTarget->GetType();

    Red::DynArray<Red::CProperty*> sourceProps;
    sourceType->GetProperties(sourceProps);

    for (const auto& sourceProp : sourceProps)
    {
        const auto targetProp = targetType->GetProperty(sourceProp->name);

        if (targetProp && targetProp->type == sourceProp->type)
        {
            targetProp->SetValue(aTarget, sourceProp->GetValuePtr<void>(aSource));
        }
    }
}
