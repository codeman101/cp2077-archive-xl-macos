#pragma once

#include "Red/EntityTemplate.hpp"

namespace Red
{
using AppearanceResource = appearance::AppearanceResource;
using AppearanceDefinition = appearance::AppearanceDefinition;
}

namespace Raw::AppearanceDefinition
{
using Mutex = Core::OffsetPtr<0xE6, Red::SharedSpinLock>;
using CompilationFlag = Core::OffsetPtr<0xE7, bool>;
using CompilationJob = Core::OffsetPtr<0xE8, Red::JobHandle>;

#if defined(__APPLE__)
// Mac (R22, HIGH): the DynArray comes back through x8 and the Windows out argument is gone; a HookAfter callback (DynArray&, token) is the
// tail observer form of the hook template.
constexpr auto ExtractPartComponents = Core::RawFunc<
    /* addr = */ Red::AddressLib::AppearanceDefinition_ExtractPartComponents,
    /* type = */ Red::DynArray<Red::Handle<Red::ISerializable>> (*)(
        const Red::SharedPtr<Red::ResourceToken<Red::EntityTemplate>>& aPartToken)>();
#else
constexpr auto ExtractPartComponents = Core::RawFunc<
    /* addr = */ Red::AddressLib::AppearanceDefinition_ExtractPartComponents,
    /* type = */ void* (*)(Red::DynArray<Red::Handle<Red::ISerializable>>& aOut,
                           const Red::SharedPtr<Red::ResourceToken<Red::EntityTemplate>>& aPartToken)>();
#endif
}

namespace Raw::AppearanceResource
{
using Mutex = Core::OffsetPtr<0xF0, Red::SharedSpinLock>;

// Mac (R22, HIGH): the virtual takes (this, PostLoadParams*) on the Mac as well; upstream now declares both arguments.
constexpr auto PostLoad = Core::RawFunc<
    /* addr = */ Red::AddressLib::AppearanceResource_PostLoad,
    /* type = */ void (*)(Red::AppearanceResource* aResource, Red::PostLoadParams* a2)>();

#if defined(__APPLE__)
// Mac ABI (R22, Mac 0xAD7048): the Handle comes back through x8 and the out argument is gone; the three arguments after the resource keep their order.
constexpr auto FindAppearance = Core::RawFunc<
    /* addr = */ Red::AddressLib::AppearanceResource_FindAppearanceDefinition,
    /* type = */ Red::Handle<Red::AppearanceDefinition> (*)(Red::AppearanceResource* aResource,
                                                            Red::CName aName,
                                                            uint32_t,
                                                            uint8_t)>();
#else
constexpr auto FindAppearance = Core::RawFunc<
    /* addr = */ Red::AddressLib::AppearanceResource_FindAppearanceDefinition,
    /* type = */ uintptr_t (*)(Red::AppearanceResource* aResource,
                               Red::Handle<Red::AppearanceDefinition>* aDefinition,
                               Red::CName aName,
                               uint32_t,
                               uint8_t)>();
#endif
}
