#pragma once

#include "TypeInfo/Resolving.hpp"

template<>
struct std::hash<RED4ext::CName>
{
    std::size_t operator()(RED4ext::CName aKey) const
    {
        return aKey.hash;
    }
};

template<>
struct std::hash<RED4ext::TweakDBID>
{
    std::size_t operator()(RED4ext::TweakDBID aKey) const
    {
#if defined(__APPLE__)
        // R26. operator== compares the hash and the length only, because the bytes 5 to 7 of a TweakDBID that comes out of game memory can hold the
        // offset that the game caches there. The hash has to ignore them as well, or two ids that are equal land in different buckets.
        return static_cast<std::size_t>(aKey.value & 0xFFFFFFFFFFull);
#else
        return aKey.value;
#endif
    }
};

template<>
struct std::hash<RED4ext::ResourcePath>
{
    std::size_t operator()(RED4ext::ResourcePath aKey) const
    {
        return aKey.hash;
    }
};

template<>
struct std::hash<RED4ext::NodeRef>
{
    std::size_t operator()(RED4ext::NodeRef aKey) const
    {
        return aKey.hash;
    }
};

template<typename T>
requires std::is_class_v<T> && std::is_convertible_v<T, uint64_t> && Red::Detail::HasGeneratedTypeName<T>
struct std::hash<T>
{
    std::size_t operator()(T aKey) const
    {
        return static_cast<uint64_t>(aKey);
    }
};
