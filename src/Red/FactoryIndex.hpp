#pragma once

namespace Raw::FactoryIndex
{
constexpr auto LoadFactoryAsync = Core::RawFunc<
    /* addr = */ Red::AddressLib::FactoryIndex_LoadFactoryAsync,
    /* type = */ void (*)(uintptr_t aIndex, Red::ResourcePath aPath, uintptr_t aContext)>();

#if defined(__APPLE__)
// Mac (R22, Mac 0xCC0BEC): the map find is inlined and the ResourcePath hash comes back in x0 (0 when missing); the out argument is gone.
constexpr auto ResolveResource = Core::RawFunc<
    /* addr = */ Red::AddressLib::FactoryIndex_ResolveResource,
    /* type = */ uint64_t (*)(uintptr_t aIndex, Red::CName aName)>();
#else
constexpr auto ResolveResource = Core::RawFunc<
    /* addr = */ Red::AddressLib::FactoryIndex_ResolveResource,
    /* type = */ void (*)(uintptr_t aIndex, Red::ResourcePath& aPath, Red::CName aName)>();
#endif
}
