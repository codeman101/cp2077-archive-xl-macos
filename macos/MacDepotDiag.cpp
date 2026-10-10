#include "MacDepotDiag.hpp"
#include "MacDiag.hpp"

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <mach/mach.h>
#include <mach/mach_vm.h>

bool Mac::SafeRead(const void* aSrc, void* aDst, size_t aSize)
{
    if (!aSrc)
        return false;
    mach_vm_size_t got = 0;
    const auto kr = mach_vm_read_overwrite(mach_task_self(), reinterpret_cast<mach_vm_address_t>(aSrc), aSize,
                                           reinterpret_cast<mach_vm_address_t>(aDst), &got);
    return kr == KERN_SUCCESS && got == aSize;
}

std::string Mac::SafeCString(const void* aCString)
{
    uint8_t raw[0x20];
    if (!SafeRead(aCString, raw, sizeof(raw)))
        return "<unreadable>";

    uint32_t length = 0;
    std::memcpy(&length, raw + 0x14, sizeof(length));
    const uint32_t size = length & 0x3FFFFFFF;
    const bool heap = (length & 0x40000000) != 0;
    if (size == 0)
        return "";
    if (size > 4096)
        return "<length " + std::to_string(size) + ">";

    char buf[4100];
    if (!heap)
    {
        // Inline text: at most 0x14 bytes including the terminator.
        const uint32_t n = size < 0x13 ? size : 0x13;
        std::memcpy(buf, raw, n);
        buf[n] = 0;
        return buf;
    }

    uintptr_t ptr = 0;
    std::memcpy(&ptr, raw, sizeof(ptr));
    if (!SafeRead(reinterpret_cast<const void*>(ptr), buf, size))
        return "<bad pointer>";
    buf[size] = 0;
    return buf;
}

namespace
{
void DumpWords(const char* aLabel, const void* aAddr, size_t aBytes)
{
    uint64_t words[32] = {};
    if (aBytes > sizeof(words))
        aBytes = sizeof(words);
    if (!Mac::SafeRead(aAddr, words, aBytes))
    {
        std::fprintf(stderr, "[axl] depot %s %p unreadable\n", aLabel, aAddr);
        return;
    }
    for (size_t i = 0; i < aBytes / 8; i += 4)
    {
        std::fprintf(stderr, "[axl] depot %s +%02zx: %016llx %016llx %016llx %016llx\n", aLabel, i * 8,
                     static_cast<unsigned long long>(words[i]), static_cast<unsigned long long>(i + 1 < aBytes / 8 ? words[i + 1] : 0),
                     static_cast<unsigned long long>(i + 2 < aBytes / 8 ? words[i + 2] : 0),
                     static_cast<unsigned long long>(i + 3 < aBytes / 8 ? words[i + 3] : 0));
    }
}
}

void Mac::DumpDepot(const void* aDepot, const char* aWhere)
{
    if (!Mac::DiagEnabled())
        return;

    std::fprintf(stderr, "[axl] depot dump (%s) depot=%p\n", aWhere, aDepot);
    DumpWords("object", aDepot, 0x90);

    struct DynArrayRaw
    {
        uint64_t entries;
        uint32_t capacity;
        uint32_t size;
    } groups{};
    if (!SafeRead(static_cast<const uint8_t*>(aDepot) + 0x10, &groups, sizeof(groups)))
        return;
    std::fprintf(stderr, "[axl] depot groups entries=%llx capacity=%u size=%u\n", static_cast<unsigned long long>(groups.entries),
                 groups.capacity, groups.size);
    if (groups.size > 64)
        return;

    int archiveDumps = 0;
    for (uint32_t g = 0; g < groups.size; ++g)
    {
        const auto* group = reinterpret_cast<const uint8_t*>(groups.entries) + g * 0x38ull;
        DumpWords("group", group, 0x38);

        DynArrayRaw archives{};
        uint32_t scope = 0;
        SafeRead(group, &archives, sizeof(archives));
        SafeRead(group + 0x30, &scope, sizeof(scope));
        std::fprintf(stderr, "[axl] depot group %u scope=%u base=\"%s\" archives=%u (capacity %u)\n", g, scope,
                     SafeCString(group + 0x10).c_str(), archives.size, archives.capacity);

        if (archives.size > 0 && archives.size < 4096 && archiveDumps < 6)
        {
            for (uint32_t a = 0; a < archives.size && a < 2; ++a)
            {
                const auto* archive = reinterpret_cast<const uint8_t*>(archives.entries) + a * 0x50ull;
                DumpWords("archive", archive, 0x50);
                std::fprintf(stderr, "[axl] depot   archive %u path=\"%s\"\n", a, SafeCString(archive + 0x10).c_str());
                ++archiveDumps;
            }
        }
    }
}
