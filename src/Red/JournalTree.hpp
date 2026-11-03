#pragma once

namespace Raw::JournalTree
{
constexpr auto ProcessJournalIndex = Core::RawFunc<
    /* addr = */ Red::AddressLib::JournalTree_ProcessJournalIndex,
#ifdef __APPLE__
    // Mac 2.5 (0x52360B4) takes two arguments: it keeps x0 and x1 and overwrites x2 before reading it.
    /* type = */ void (*)(uintptr_t a1, Red::JobGroup& aJobGroup)>();
#else
    /* type = */ void (*)(uintptr_t a1, uintptr_t a2, Red::JobQueue& aJobQueue)>();
#endif
}

namespace Raw::JournalRootFolderEntry
{
constexpr auto Initialize = Core::RawFunc<
    /* addr = */ Red::AddressLib::JournalRootFolderEntry_Initialize,
    /* type = */ void (*)(Red::game::JournalRootFolderEntry* aRoot,
                          uintptr_t a2, uintptr_t a3, Red::JobQueue& aJobQueue)>();
}
