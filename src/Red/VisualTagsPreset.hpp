#pragma once

#include "Red/TagList.hpp"

namespace Red
{
using AppearanceNameVisualTagsPreset = game::AppearanceNameVisualTagsPreset;
}

namespace Raw::AppearanceNameVisualTagsPreset
{
constexpr auto GetVisualTags = Core::RawFunc<
    /* addr = */ Red::AddressLib::AppearanceNameVisualTagsPreset_GetVisualTags,
    /* type = */ void (*)(Red::AppearanceNameVisualTagsPreset* aPreset, // FIXME: might not be a preset anymore
                          Red::ResourcePath aEntityPath,
                          Red::CName aAppearanceName,
                          Red::TagList& aOutTags)>();

#ifdef __APPLE__
// The record-based entry. Windows 0x3F12F0 is a 68-byte wrapper that reads the entity path from the flat its record's
// TweakDBID at +0xE8 names and tail-jumps into GetVisualTags 0x3F1334, so hooking GetVisualTags covers it there. The Mac
// twin 0x3CD7118 (284 bytes) reads the TweakDBID at +0xE4, has the map find inlined and branches into the shared value
// method 0x3CD2F60 (twin of Windows 0x3F1380) without passing through GetVisualTags 0x3CD7234, so a record-based call
// never reaches the GetVisualTags hook on macOS. Arguments as the real caller 0x3CD7BB8 sets them: x0 preset, x1 record,
// x2 appearance name, x3 tag list.
constexpr auto GetVisualTagsByRecord = Core::RawFunc<
    /* addr = */ Red::AddressLib::AppearanceNameVisualTagsPreset_GetVisualTagsByRecord,
    /* type = */ void (*)(Red::AppearanceNameVisualTagsPreset* aPreset,
                          const uint8_t* aRecord,
                          Red::CName aAppearanceName,
                          Red::TagList& aOutTags)>();
#endif
}
