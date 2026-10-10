#pragma once

namespace Raw::ItemObject
{
using ItemID = Core::OffsetPtr<0x288, Red::ItemID>;

#if defined(__APPLE__)
// Mac (R22): vtable slot 0x288 (Windows 0x280 plus the second destructor slot) is 0x373A494, `ldr x0,[x0,#0x50]; ret`: the CName comes back in x0.
constexpr auto GetAppearanceName = Core::RawVFunc<
    /* offset = */ 0x280,
    /* type = */ Red::CName (Red::ItemObject::*)(),
    /* verified on Mac = */ true>();
#else
constexpr auto GetAppearanceName = Core::RawVFunc<
    /* offset = */ 0x280,
    /* type = */ Red::CName* (Red::ItemObject::*)(Red::CName& aOut)>();
#endif
}
