#pragma once

#include <atomic>

#include "Core/Foundation/Feature.hpp"
#include "Core/Hooking/HookingAgent.hpp"
#include "Core/Logging/LoggingAgent.hpp"
#include "Red/EntitySpawner.hpp"

namespace App
{
class EntitySpawnerPatch
    : public Core::Feature
    , public Core::HookingAgent
    , public Core::LoggingAgent
{
protected:
    void OnBootstrap() override;

#if defined(__APPLE__)
    // macOS: the spawner result (a SharedPtr) comes back through x8, so the hook has no out argument.
    static void OnSpawnEntity(void* aSpawner, Red::EntitySpawnerRequest* aRequest, Red::ResourcePath aTemplate);

    // R22 (CODEWARE_MAC_DIAG=1, read-only): how many spawns the hook saw and how many it gave a record appearance name.
    static void LogMacStats(const char* aWhere);
    inline static std::atomic<uint32_t> s_spawnHits{0};
    inline static std::atomic<uint32_t> s_spawnPatched{0};
#else
    static void OnSpawnEntity(void* aSpawner, void* aOut, Red::EntitySpawnerRequest* aRequest,
                              Red::ResourcePath aTemplate);
#endif
};
}
