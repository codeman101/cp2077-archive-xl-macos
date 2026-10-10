#pragma once

namespace Red
{
struct MeshMaterialsData
{
    using AllocatorType = Memory::MeshAllocator;

    DynArray<Handle<IMaterial>> materials;
};

struct MeshMaterialsToken
{
    JobHandle job;
    SharedPtr<MeshMaterialsData> data;
};
}

namespace Raw::CMesh
{
using MaterialLock = Core::OffsetPtr<0x218, Red::SharedSpinLock>;

constexpr auto PostLoad = Core::RawFunc<
    /* addr = */ Red::AddressLib::CMesh_PostLoad,
    /* type = */ void (*)(Red::CMesh* aResource, Red::PostLoadParams* a2)>();

constexpr auto GetAppearance = Core::RawFunc<
    /* addr = */ Red::AddressLib::CMesh_GetAppearance,
    /* type = */ Red::Handle<Red::mesh::MeshAppearance>& (*)(Red::CMesh* aMesh, Red::CName aAppearance)>();

constexpr auto FindAppearance = Core::RawFunc<
    /* addr = */ Red::AddressLib::CMesh_FindAppearance,
    /* type = */ Red::Handle<Red::mesh::MeshAppearance>& (*)(Red::CMesh* aMesh, Red::CName aAppearance)>();

#if defined(__APPLE__)
// Mac (R22): the token comes back in x8 (non-trivial destructor), the Windows out argument disappears.
constexpr auto LoadMaterialsAsync = Core::RawFunc<
    /* addr = */ Red::AddressLib::CMesh_LoadMaterialsAsync,
    /* type = */ Red::MeshMaterialsToken (*)(Red::CMesh* aMesh,
                                             const Red::DynArray<Red::CName>& aMaterialNames,
                                             uint8_t a4)>();

// Mac only: the action CMesh::PostLoad takes after its inlined preload test (mesh, a flag that is 0, the load request).
constexpr auto PreloadAppearances = Core::RawFunc<
    /* addr = */ Red::AddressLib::CMesh_PreloadAppearances,
    /* type = */ void (*)(Red::CMesh* aMesh, uint32_t a2, void* aRequest)>();
#else
constexpr auto LoadMaterialsAsync = Core::RawFunc<
    /* addr = */ Red::AddressLib::CMesh_LoadMaterialsAsync,
    /* type = */ void* (*)(Red::CMesh* aMesh,
                           Red::MeshMaterialsToken& aOut,
                           const Red::DynArray<Red::CName>& aMaterialNames,
                           uint8_t a4)>();
#endif

constexpr auto AddStubAppearance = Core::RawFunc<
    /* addr = */ Red::AddressLib::CMesh_AddStubAppearance,
    /* type = */ void (*)(Red::CMesh* aMesh)>();

constexpr auto ShouldPreloadAppearances = Core::RawFunc<
    /* addr = */ Red::AddressLib::CMesh_ShouldPreloadAppearances,
    /* type = */ bool (*)(Red::CMesh* aMesh)>();
}

namespace Raw::MeshMaterialBuffer
{
#if defined(__APPLE__)
// Mac (R22): the token comes back in x8; the other five arguments fit the registers.
constexpr auto LoadMaterialAsync = Core::RawFunc<
    /* addr = */ Red::AddressLib::MeshMaterialBuffer_LoadMaterialAsync,
    /* type = */ Red::SharedPtr<Red::ResourceToken<Red::IMaterial>> (*)(Red::meshMeshMaterialBuffer* aBuffer,
                                                                        const Red::Handle<Red::CMesh>& aMesh,
                                                                        uint16_t aMaterialIndex,
                                                                        uint64_t a5,
                                                                        uint8_t a6)>();
#else
constexpr auto LoadMaterialAsync = Core::RawFunc<
    /* addr = */ Red::AddressLib::MeshMaterialBuffer_LoadMaterialAsync,
    /* type = */ void* (*)(Red::meshMeshMaterialBuffer* aBuffer,
                           Red::SharedPtr<Red::ResourceToken<Red::IMaterial>>& aOut,
                           const Red::Handle<Red::CMesh>& aMesh,
                           uint16_t aMaterialIndex,
                           uint64_t a5,
                           uint8_t a6)>();
#endif
}

namespace Raw::MeshAppearance
{
using Owner = Core::OffsetPtr<0x50, Red::CMesh*>;

#if defined(__APPLE__)
// Mac (R22): the handle comes back in x8 and the appearance is passed by pointer (as in Codeware's R21 prototype).
constexpr auto LoadMaterialSetupAsync = Core::RawFunc<
    /* addr = */ Red::AddressLib::MeshAppearance_LoadMaterialSetupAsync,
    /* type = */ Red::Handle<Red::mesh::MeshAppearance> (*)(Red::mesh::MeshAppearance* aAppearance, uint8_t a3)>();
#else
constexpr auto LoadMaterialSetupAsync = Core::RawFunc<
    /* addr = */ Red::AddressLib::MeshAppearance_LoadMaterialSetupAsync,
    /* type = */ void (*)(Red::mesh::MeshAppearance& aAppearance, Red::Handle<Red::mesh::MeshAppearance>& aOut,
                          uint8_t a3)>();
#endif
}

namespace Raw::MeshComponent
{
// Mac slots are Windows + 8 (RawVFunc adds it); LoadResource 0x268 and RefreshAppearance 0x288 are confirmed on entMeshComponent and entSkinnedMeshComponent.
constexpr auto LoadResource = Core::RawVFunc<
        /* offset = */ 0x260,
        /* type = */ uint64_t(Red::IComponent::*)(Red::JobQueue& aQueue)
#if defined(__APPLE__)
        , /* verified on Mac = */ true
#endif
        >();

constexpr auto RefreshAppearance = Core::RawVFunc<
        /* offset = */ 0x280,
        /* type = */ void(Red::IComponent::*)()
#if defined(__APPLE__)
        , /* verified on Mac = */ true
#endif
        >();
}
