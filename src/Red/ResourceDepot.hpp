#pragma once

#if defined(__APPLE__)
namespace Red
{
// Mac (R22): the 16 byte resource handle ({pointer, reference block}) comes back through x8. The user-provided destructor makes the type
// non-trivial for calls, which is what puts it in x8 in clang as in the game. Ownership is forwarded to the caller, never released here.
struct DepotResourceHandle
{
    DepotResourceHandle() = default;
    DepotResourceHandle(const DepotResourceHandle&) = default;
    DepotResourceHandle& operator=(const DepotResourceHandle&) = default;
    ~DepotResourceHandle() {}

    uint64_t ptr{};   // 00
    uint64_t block{}; // 08
};
RED4EXT_ASSERT_SIZE(DepotResourceHandle, 0x10);
}
#endif

namespace Raw::ResourceDepot
{
constexpr auto InitializeArchives = Core::RawFunc<
    /* addr = */ Red::AddressLib::ResourceDepot_InitializeArchives,
    /* type = */ void (*)(Red::ResourceDepot* aDepot)>{};

#if defined(__APPLE__)
// Mac (R22): the depot is not used by the function (inlined away), x1 is the group, x2 the path list, x3 the loaded resource paths,
// w4 the memory-resident flag and w5 a mode that only appears in the failure assertions (the game passes 2 everywhere).
// A bad archive is a fatal assertion in the game ("Failed to create the archive"), as on Windows.
constexpr auto LoadArchives = Core::RawFunc<
    /* addr = */ Red::AddressLib::ResourceDepot_LoadArchives,
    /* type = */ void (*)(Red::ResourceDepot* aDepot,
                          Red::ArchiveGroup& aGroup,
                          const Red::DynArray<Red::CString>& aArchivePaths,
                          Red::DynArray<Red::ResourcePath>& aLoadedResourcePaths,
                          bool aMemoryResident,
                          uint32_t aMode)>{};
constexpr uint32_t LoadArchivesMode = 2;
#else
constexpr auto LoadArchives = Core::RawFunc<
    /* addr = */ Red::AddressLib::ResourceDepot_LoadArchives,
    /* type = */ void (*)(Red::ResourceDepot* aDepot,
                          Red::ArchiveGroup& aGroup,
                          const Red::DynArray<Red::CString>& aArchivePaths,
                          Red::DynArray<Red::ResourcePath>& aLoadedResourcePaths,
                          uint32_t aMemoryResident,
                          uint8_t aMode)>{}; // 2.5: base archives pass 2, mods pass 0
#endif

#if defined(__APPLE__)
constexpr auto RequestResource = Core::RawFunc<
    /* addr = */ Red::AddressLib::ResourceDepot_RequestResource,
    /* type = */ Red::DepotResourceHandle (*)(Red::ResourceDepot* aDepot,
                                              Red::ResourcePath aPath,
                                              const int32_t* aArchiveHandle)>{};
#else
constexpr auto RequestResource = Core::RawFunc<
    /* addr = */ Red::AddressLib::ResourceDepot_RequestResource,
    /* type = */ uintptr_t* (*)(Red::ResourceDepot* aDepot,
                                const uintptr_t* aOutResourceHandle,
                                Red::ResourcePath aPath,
                                const int32_t* aArchiveHandle)>{};
#endif

constexpr auto CheckResource = Core::RawFunc<
    /* addr = */ Red::AddressLib::ResourceDepot_CheckResource,
    /* type = */ bool (*)(Red::ResourceDepot* aDepot, Red::ResourcePath aPath)>{};
}
