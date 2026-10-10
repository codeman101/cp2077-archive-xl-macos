#pragma once

namespace Raw::MappinSystem
{
using CookedMappinResource = Core::OffsetPtr<0x58, Red::Handle<Red::gameMappinResource>>;
using CookedPoiResource = Core::OffsetPtr<0x68, Red::Handle<Red::gamePointOfInterestMappinResource>>;

#if defined(__APPLE__)
// Mac (R22): the finders behind the 16 byte getter wrappers 0x42A3E38 (mappin resource) and 0x42A3E68 (POI resource). The first argument is
// the cooked resource itself (the getter loads it from this+0x58 / +0x68), the result is the entry of the cooked data array or null.
constexpr auto FindMappinData = Core::RawFunc<
    /* addr = */ Red::AddressLib::MappinSystem_FindMappinData,
    /* type = */ void* (*)(void* aResource, uint32_t aHash)>();

constexpr auto FindPoiData = Core::RawFunc<
    /* addr = */ Red::AddressLib::MappinSystem_FindPoiData,
    /* type = */ void* (*)(void* aResource, uint32_t aHash)>();
#endif

constexpr auto GetMappinData = Core::RawFunc<
    /* addr = */ Red::AddressLib::MappinSystem_GetMappinData,
    /* type = */ void* (*)(void* aSystem, uint32_t aHash)>();

constexpr auto GetPoiData = Core::RawFunc<
    /* addr = */ Red::AddressLib::MappinSystem_GetPoiData,
    /* type = */ void* (*)(void* aSystem, uint32_t aHash)>();

constexpr auto OnStreamingWorldLoaded = Core::RawFunc<
    /* addr = */ Red::AddressLib::MappinSystem_OnStreamingWorldLoaded,
    /* type = */ void (*)(void* aSystem, Red::worldRuntimeScene*)>();
}
