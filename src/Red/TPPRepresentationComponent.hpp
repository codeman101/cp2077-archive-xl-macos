#pragma once

namespace Raw::TPPRepresentationComponent
{
using SlotListener = Core::OffsetPtr<0x148, Red::Handle<Red::game::IAttachmentSlotsListener>>;

constexpr auto OnAttach = Core::RawFunc<
    /* addr = */ Red::AddressLib::TPPRepresentationComponent_OnAttach,
    /* type = */ void (*)(Red::game::TPPRepresentationComponent* aComponent, uintptr_t a2)>();

constexpr auto RegisterAffectedItem = Core::RawFunc<
    /* addr = */ Red::AddressLib::TPPRepresentationComponent_RegisterAffectedItem,
    /* type = */ void (*)(Red::game::TPPRepresentationComponent* aComponent,
                          Red::TweakDBID aItemID,
                          const Red::Handle<Red::ItemObject>& aItemObject)>();

#if defined(__APPLE__)
// macOS (R22): IsAffectedSlot (Windows 0x9FE870, two callers) is inlined into OnItemEquipped and OnItemUnequipped, against a two entry table
// (Head, Eyes) at 0x8768490. The extra slots are handled by hooks on those two functions: (component, item id, slot id), void, both ids in registers.
constexpr auto OnItemEquipped = Core::RawFunc<
    /* addr = */ Red::AddressLib::TPPRepresentationComponent_OnItemEquipped,
    /* type = */ void (*)(Red::game::TPPRepresentationComponent* aComponent, Red::TweakDBID aItemID, Red::TweakDBID aSlotID)>();

constexpr auto OnItemUnequipped = Core::RawFunc<
    /* addr = */ Red::AddressLib::TPPRepresentationComponent_OnItemUnequipped,
    /* type = */ void (*)(Red::game::TPPRepresentationComponent* aComponent, Red::TweakDBID aItemID, Red::TweakDBID aSlotID)>();

constexpr auto UnregisterAffectedItem = Core::RawFunc<
    /* addr = */ Red::AddressLib::TPPRepresentationComponent_UnregisterAffectedItem,
    /* type = */ void (*)(Red::game::TPPRepresentationComponent* aComponent, Red::TweakDBID aItemID)>();
#else
constexpr auto IsAffectedSlot = Core::RawFunc<
    /* addr = */ Red::AddressLib::TPPRepresentationComponent_IsAffectedSlot,
    /* type = */ bool (*)(Red::TweakDBID aSlotID)>();
#endif
}
