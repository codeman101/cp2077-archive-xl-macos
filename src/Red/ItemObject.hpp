#pragma once

namespace Raw::ItemObject
{
using ItemID = Core::OffsetPtr<0x288, Red::ItemID>;

#if defined(__APPLE__)
// Mac (R22): vtable slot 0x288 (Windows 0x280 plus the second destructor slot) is 0x373A494, `ldr x0,[x0,#0x50]; ret`: the CName comes back in x0.
// 2.5: the twin sits at vtable slot 0x290 (LAYOUT-PORTS.tsv, 14/14 votes across the vtables), so the source offset is 0x288 on the Mac.
constexpr auto GetAppearanceName = Core::RawVFunc<
    /* offset = */ 0x288,
    /* type = */ Red::CName (Red::ItemObject::*)(),
    /* verified on Mac = */ true>();
#else
constexpr auto GetAppearanceName = Core::RawVFunc<
    /* offset = */ 0x280,
    /* type = */ Red::CName* (Red::ItemObject::*)(Red::CName& aOut)>();
#endif
}
