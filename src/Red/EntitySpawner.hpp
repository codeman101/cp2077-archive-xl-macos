#pragma once

#include "Red/Common.hpp"

namespace Red
{
#if defined(__APPLE__)
// macOS (R22, /tmp/axl-d-signatures.md 1.1): the request embeds a libc++ std::function (0x20 bytes) where the Windows one has a 0x40 byte
// block, so every member behind it is 0x20 lower on the Mac. Size 0xD0 (Windows 0xF0).
struct EntitySpawnerRequest
{
    uint8_t unk00[0xA0];        // 00
    CName appearanceName;       // A0
    uint8_t unkA8[0xC0 - 0xA8]; // A8
    TweakDBID recordID;         // C0
};
RED4EXT_ASSERT_OFFSET(EntitySpawnerRequest, appearanceName, 0xA0);
RED4EXT_ASSERT_OFFSET(EntitySpawnerRequest, recordID, 0xC0);
#else
struct EntitySpawnerRequest
{
    uint8_t unk00[0xC0];        // 00
    CName appearanceName;       // C0
    uint8_t unkC8[0xE0 - 0xC8]; // C8
    TweakDBID recordID;         // E0
};
RED4EXT_ASSERT_OFFSET(EntitySpawnerRequest, appearanceName, 0xC0);
RED4EXT_ASSERT_OFFSET(EntitySpawnerRequest, recordID, 0xE0);
#endif
}

namespace Raw::EntitySpawner
{
#if defined(__APPLE__)
// Mac ABI (R22, Mac 0x14084B0): x0 = spawner, x1 = request, x2 = template path, and the spawn ticket (one owning pointer to an object every
// caller releases through its vtable) comes back through x8. The Raw type must return a type that clang also returns through x8, otherwise the
// detour is free to clobber x8 before the original runs. The bytes are forwarded untouched and never released here.
using SpawnTicket = Red::SretValue<8>;

constexpr auto SpawnFromTemplate = Core::RawFunc<
    /* addr = */ Red::AddressLib::EntitySpawner_SpawnFromTemplate,
    /* type = */ SpawnTicket (*)(void* aSpawner, Red::EntitySpawnerRequest* aRequest, Red::ResourcePath aTemplate)>();
#else
constexpr auto SpawnFromTemplate = Core::RawFunc<
    /* addr = */ Red::AddressLib::EntitySpawner_SpawnFromTemplate,
    /* type = */ void* (*)(void* aSpawner, void* aOut, Red::EntitySpawnerRequest* aRequest,
                           Red::ResourcePath aTemplate)>();
#endif
}
