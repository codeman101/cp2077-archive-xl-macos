#pragma once

namespace Red
{
using JournalEntryHash = uint32_t;
}

namespace Raw::JournalManager
{
#if defined(__APPLE__)
// Mac ABI (R22, read-only matcher, HIGH): a JobHandle comes back through x8 and the Windows `JobHandle& aJobHandle` argument is gone.
constexpr auto LoadJournal = Core::RawFunc<
    /* addr = */ Red::AddressLib::JournalManager_LoadJournal,
    /* type = */ Red::JobHandle (*)(Red::IScriptable* aManager)>();

// Mac vtable slots: Windows + 8 (RawVFunc adds it) except GetEntryByHash, whose overload order is reversed (Mac slot 0x220, so A = 0x218).
// A Handle result comes back through x8 and the Windows out argument is gone.
constexpr auto GetTrackedQuest = Core::RawVFunc<
    /* addr = */ 0x1F8,
    /* type = */ Red::Handle<Red::game::JournalEntry> (Red::gameIJournalManager::*)(),
    /* verified on Mac = */ true>();

constexpr auto GetTrackedPointOfInterest = Core::RawVFunc<
    /* addr = */ 0x208,
    /* type = */ Red::Handle<Red::game::JournalEntry> (Red::gameIJournalManager::*)(),
    /* verified on Mac = */ true>();

constexpr auto GetEntryByHash = Core::RawVFunc<
    /* addr = */ 0x218,
    /* type = */ Red::Handle<Red::gameJournalEntry> (Red::gameIJournalManager::*)(Red::JournalEntryHash aEntryHash),
    /* verified on Mac = */ true>();

constexpr auto GetEntryHash = Core::RawVFunc<
    /* addr = */ 0x230,
    /* type = */ Red::JournalEntryHash (Red::gameIJournalManager::*)(Red::Handle<Red::game::JournalEntry>& aEntry),
    /* verified on Mac = */ true>();
#else
constexpr auto LoadJournal = Core::RawFunc<
    /* addr = */ Red::AddressLib::JournalManager_LoadJournal,
    /* type = */ void (*)(Red::IScriptable* aManager, Red::JobHandle& aJobHandle)>();

constexpr auto GetTrackedQuest = Core::RawVFunc<
    /* addr = */ 0x1F8,
    /* type = */ void (Red::gameIJournalManager::*)(Red::Handle<Red::game::JournalEntry>& aEntry)>();

constexpr auto GetTrackedPointOfInterest = Core::RawVFunc<
    /* addr = */ 0x208,
    /* type = */ void (Red::gameIJournalManager::*)(Red::Handle<Red::game::JournalEntry>& aEntry)>();

constexpr auto GetEntryByHash = Core::RawVFunc<
    /* addr = */ 0x220,
    /* type = */ void* (Red::gameIJournalManager::*)(Red::Handle<Red::gameJournalEntry>& aOut,
                                                     Red::JournalEntryHash aEntryHash)>();

constexpr auto GetEntryHash = Core::RawVFunc<
    /* addr = */ 0x230,
    /* type = */ Red::JournalEntryHash (Red::gameIJournalManager::*)(Red::Handle<Red::game::JournalEntry>& aEntry)>();

#endif

// constexpr auto ChangeEntryState = Core::RawVFunc<
//     /* addr = */ 0x268,
//     /* type = */ bool (Red::gameIJournalManager::*)(Red::Handle<Red::gameJournalEntry>& aEntry,
//                                                     Red::gameJournalEntryState aEntryState,
//                                                     Red::gameJournalNotifyOption aNotifyOption,
//                                                     uint32_t a4)>();

constexpr auto TrackQuest = Core::RawFunc<
    /* addr = */ Red::AddressLib::JournalManager_TrackQuest,
    /* type = */ void (Red::gameIJournalManager::*)(Red::Handle<Red::game::JournalEntry>& aEntry)>();

#if defined(__APPLE__)
constexpr auto TrackQuestByPath = Core::RawVFunc<
    /* addr = */ 0x298,
    /* type = */ void (Red::gameIJournalManager::*)(void* aPath),
    /* verified on Mac = */ true>();

constexpr auto TrackPointOfInterest = Core::RawVFunc<
    /* addr = */ 0x2A0,
    /* type = */ void (Red::gameIJournalManager::*)(Red::Handle<Red::game::JournalEntry>& aEntry),
    /* verified on Mac = */ true>();
#else
constexpr auto TrackQuestByPath = Core::RawVFunc<
    /* addr = */ 0x298,
    /* type = */ void (Red::gameIJournalManager::*)(void* aPath)>();

constexpr auto TrackPointOfInterest = Core::RawVFunc<
    /* addr = */ 0x2A0,
    /* type = */ void (Red::gameIJournalManager::*)(Red::Handle<Red::game::JournalEntry>& aEntry)>();
#endif

// constexpr auto GetEntryState = Core::RawVFunc<
//     /* addr = */ 0x2B0,
//     /* type = */ Red::gameJournalEntryState (Red::gameIJournalManager::*)(Red::Handle<Red::gameJournalEntry>& aEntry)>();
//
// constexpr auto GetEntryTimestamp = Core::RawVFunc<
//     /* addr = */ 0x2C8,
//     /* type = */ uint32_t (Red::gameIJournalManager::*)(Red::Handle<Red::gameJournalEntry>& aEntry)>();
}
