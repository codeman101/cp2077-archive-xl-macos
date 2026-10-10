#include "EntitySpawnerPatch.hpp"
#include "Red/TweakDB.hpp"
#if defined(__APPLE__)
#include "App/Utils/MacProbe.hpp"
#include "MacDiag.hpp"
#endif

namespace
{
constexpr auto DefaultAppearanceName = Red::CName("default");
}

void App::EntitySpawnerPatch::OnBootstrap()
{
    HookBefore<Raw::EntitySpawner::SpawnFromTemplate>(&OnSpawnEntity).OrThrow();

#if defined(__APPLE__)
    App::MacProbe::AddStats(&LogMacStats);
#endif
}

#if defined(__APPLE__)
void App::EntitySpawnerPatch::LogMacStats(const char* aWhere)
{
    MAC_DIAG("[selftest] R22 EntitySpawner %s: SpawnFromTemplate hook ran %u times, %u requests got the record appearance name\n", aWhere,
             s_spawnHits.load(), s_spawnPatched.load());
}
#endif

#if defined(__APPLE__)
void App::EntitySpawnerPatch::OnSpawnEntity(void* aSpawner, Red::EntitySpawnerRequest* aRequest,
                                            Red::ResourcePath aTemplate)
#else
void App::EntitySpawnerPatch::OnSpawnEntity(void* aSpawner, void* aOut, Red::EntitySpawnerRequest* aRequest,
                                            Red::ResourcePath aTemplate)
#endif
{
#if defined(__APPLE__)
    if (!aRequest)
        return;

    Mac::Count(s_spawnHits);
#endif

    if (aRequest->recordID && (!aRequest->appearanceName || aRequest->appearanceName == DefaultAppearanceName))
    {
        auto recordAppearanceName = Red::GetFlatValue<Red::CName>({aRequest->recordID, ".appearanceName"});
        if (recordAppearanceName)
        {
            aRequest->appearanceName = recordAppearanceName;
#if defined(__APPLE__)
            Mac::Count(s_spawnPatched);
#endif
        }
    }
}
