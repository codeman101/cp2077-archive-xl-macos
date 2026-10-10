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
        // Hash only what operator== compares (name hash and length), not the TweakDB value offset the game caches in
        // the top 24 bits: equal IDs must hash equally, or an ID taken from game memory (a record field, a hook
        // argument) is not found among the IDs the plugin made itself. Same as TweakXL's specialization.
        return aKey.value & 0xFFFFFFFFFFull;
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
requires std::is_class_v<T> && std::is_convertible_v<T, size_t> && Red::Detail::HasGeneratedTypeName<T>
struct std::hash<T>
{
    std::size_t operator()(T aKey) const
    {
        return static_cast<size_t>(aKey);
    }
};
