#pragma once

namespace Raw::WorldNodeInstance
{
using Transform = Core::OffsetPtr<0x30, Red::Transform>;
using Scale = Core::OffsetPtr<0x50, Red::Vector3>;
using Node = Core::OffsetPtr<0x60, Red::Handle<Red::worldNode>>;
}

namespace Raw::WorldNodeRegistry
{
#if defined(__APPLE__)
// Mac (R22, HIGH): Windows slot 0x190 is Mac slot 0x198 (0x33425AC, body twin of Windows 0x52556C); the Handle comes back through x8.
constexpr auto FindNode = Core::RawVFunc<
    /* addr = */ 0x190,
    /* type = */ Red::Handle<Red::worldINodeInstance> (Red::worldNodeInstanceRegistry::*)(uint64_t aNodeID),
    /* verified on Mac = */ true>();
#else
constexpr auto FindNode = Core::RawVFunc<
    /* addr = */ 0x190,
    /* type = */ void (Red::worldNodeInstanceRegistry::*)(Red::Handle<Red::worldINodeInstance>& aOut,
                                                          uint64_t aNodeID)>();
#endif
}
