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

#if defined(__APPLE__)
// Mac only (R22): twin of the Windows wrapper 0x3F12F0. The first argument after the preset is a TweakDB record whose TweakDBID at +0xE4
// (+0xE8 on Windows) names the flat that holds the entity path. The map find is inlined, so this entry does not reach GetVisualTags.
constexpr auto GetVisualTagsByRecord = Core::RawFunc<
    /* addr = */ Red::AddressLib::AppearanceNameVisualTagsPreset_GetVisualTagsByRecord,
    /* type = */ void (*)(Red::AppearanceNameVisualTagsPreset* aPreset,
                          const uint8_t* aRecord,
                          Red::CName aAppearanceName,
                          Red::TagList& aOutTags)>();
#endif
}
