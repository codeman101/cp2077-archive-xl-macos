#pragma once

#include <atomic>

#include "App/Extensions/ExtensionBase.hpp"
#include "App/Extensions/Garment/Dynamic.hpp"
#include "App/Extensions/Garment/States.hpp"
#include "App/Extensions/Garment/Config.hpp"
#include "Red/AppearanceChanger.hpp"
#include "Red/EntityTemplate.hpp"
#include "Red/GarmentAssembler.hpp"
#include "Red/VisualTagsPreset.hpp"

namespace App
{
class GarmentExtension : public ConfigurableExtensionImpl<GarmentOverrideConfig>
{
public:
    static constexpr auto ForceHairTag = Red::CName("force_Hair");
    static constexpr auto ForceFlatFeetTag = Red::CName("force_FlatFeet");

    bool Load() override;
    bool Unload() override;
    void Configure() override;
    std::string_view GetName() override;
#if defined(__APPLE__)
    void OnTweakDBReady() override;
#endif

    static void EnableGarmentOffsets();
    static void DisableGarmentOffsets();

    static Core::SharedPtr<DynamicAppearanceController>& GetDynamicAppearanceController();
    static Core::SharedPtr<OverrideTagManager>& GetTagManager();

#if defined(__APPLE__)
    // R22 (CODEWARE_MAC_DIAG=1, read-only): how often each hook ran.
    static void LogMacStats(const char* aWhere);
#endif

private:
#if defined(__APPLE__)
    enum Hit : size_t
    {
        HitLoadAppearance,
        HitChangeAppearance,
        HitResolveAppearance,
        HitResolveDefinition,
        HitSuffixes,
        HitVisualTags,
        HitVisualTagsByRecord,
        HitFindState,
        HitAddItem,
        HitAddCustomItem,
        HitChangeItem,
        HitChangeCustomItem,
        HitRemoveItem,
        HitRegisterPart,
        HitProcessGarment,
        HitProcessMesh,
        HitComputeGarment,
        HitReassemble,
        HitGameDetach,
        HitCount
    };
    inline static std::atomic<uint32_t> s_hits[HitCount];
#endif

    static void OnLoadAppearanceResource(Red::ItemFactoryRequest* aRequest);
    static void OnChangeAppearanceResource(Red::ItemFactoryAppearanceChangeRequest* aRequest);
    static Red::TemplateAppearance* OnResolveAppearance(Red::EntityTemplate* aTemplate, Red::CName aSelector);
    static void OnResolveDefinition(Red::AppearanceResource* aResource,
                                    Red::Handle<Red::AppearanceDefinition>* aDefinition,
                                    Red::CName aAppearanceSelector, uint32_t a4, uint8_t a5);
#if defined(__APPLE__)
    static void OnFindDefinition(Red::Handle<Red::AppearanceDefinition>& aDefinition, Red::AppearanceResource* aResource,
                                 Red::CName aAppearanceSelector, uint32_t a4, uint8_t a5);
    static Red::CString OnResolveSuffixes(Red::Handle<Red::GameObject>& aOwner,
                                          Red::Handle<Red::GameObject>& aOwnerOverride,
                                          const Red::Handle<Red::TweakDBRecord>& aItemRecord,
                                          const Red::ItemID& aItemID);
    static void OnGetVisualTagsByRecord(Red::AppearanceNameVisualTagsPreset* aPreset, const uint8_t* aRecord,
                                        Red::CName aAppearanceName, Red::TagList& aFinalTags);
#else
    static void* OnResolveSuffixes(Red::CString& aResult,
                                   Red::Handle<Red::GameObject>& aOwner,
                                   Red::Handle<Red::GameObject>& aOwnerOverride,
                                   const Red::TweakDBRecord& aItemRecord,
                                   const Red::ItemID& aItemID);
#endif
    static void OnGetVisualTags(Red::AppearanceNameVisualTagsPreset* aPreset,
                                Red::ResourcePath aEntityPath,
                                Red::CName aAppearanceName,
                                Red::TagList& aFinalTags);
    static void OnFindState(uintptr_t, Red::GarmentAssemblerState* aState, Red::WeakHandle<Red::Entity>& aEntityWeak);
#if defined(__APPLE__)
    static void OnFindStateResult(Red::GarmentAssemblerState& aState, uintptr_t aAggregator,
                                  Red::WeakHandle<Red::Entity>& aEntityWeak);
#endif
    static void OnAddItem(Red::GarmentAssemblerState* aState, Red::GarmentItemAddRequest&);
    static void OnAddCustomItem(Red::GarmentAssemblerState* aState, Red::GarmentItemAddCustomRequest&);
#if defined(__APPLE__)
    // Mac: the hooks sit on the aggregator level wrappers, which run before FindState links the state pointer, so the entity state is found
    // through the entity itself.
    static void OnChangeItem(uintptr_t, Red::WeakHandle<Red::Entity>& aEntityWeak, Red::GarmentItemChangeRequest&);
    static void OnChangeCustomItem(uintptr_t, Red::WeakHandle<Red::Entity>& aEntityWeak, Red::GarmentItemChangeCustomRequest&);
#else
    static void OnChangeItem(Red::GarmentAssemblerState* aState, Red::GarmentItemChangeRequest&);
    static void OnChangeCustomItem(Red::GarmentAssemblerState* aState, Red::GarmentItemChangeCustomRequest&);
#endif
    static void OnRemoveItem(uintptr_t, Red::WeakHandle<Red::Entity>&, Red::GarmentItemRemoveRequest&);
    static void OnRegisterPart(uintptr_t, Red::Handle<Red::EntityTemplate>& aPart,
                               Red::Handle<Red::ComponentsStorage>& aComponents,
                               Red::Handle<Red::AppearanceDefinition>& aAppearance);
#if defined(__APPLE__)
    static Raw::GarmentAssembler::GarmentProcessorResult OnProcessGarment(uintptr_t a2, uintptr_t a3,
                                                                          Red::GarmentLoadingParams* aParams);
#else
    static uintptr_t OnProcessGarment(Red::SharedPtr<Red::GarmentProcessingContext>& aProcessor, uintptr_t a2, uintptr_t a3,
                                      Red::GarmentLoadingParams* aParams);
#endif
    static void OnProcessGarmentMesh(Raw::GarmentAssembler::ProcessMesh aCallback,
                                     Red::GarmentProcessingContext* aProcessor,
                                     uint32_t, const Red::Handle<Red::EntityTemplate>& aPartTemplate,
                                     const Red::SharedPtr<Red::ResourceToken<>>& aMeshToken,
                                     const Red::Handle<Red::IComponent>& aComponent, const Red::JobGroup& aJobGroup);
    // static int32_t OnGetBaseMeshOffset(Red::Handle<Red::IComponent>& aComponent,
    //                                    Red::Handle<Red::EntityTemplate>& aTemplate);
    static void OnComputeGarment(uintptr_t, Red::Handle<Red::Entity>&, Red::DynArray<int32_t>&,
                                 Red::SharedPtr<Red::GarmentProcessingContext>&, uintptr_t, uintptr_t, uintptr_t, bool);
    static void OnReassembleAppearance(Red::Entity*, uintptr_t, uintptr_t, uintptr_t, uintptr_t, uintptr_t);
#if defined(__APPLE__)
    static void OnGameDetach(uintptr_t, uintptr_t);
#else
    static void OnGameDetach();
#endif

    static void RegisterOffsetOverrides(Core::SharedPtr<EntityState>& aEntityState, uint64_t aHash,
                                      Red::Handle<Red::AppearanceDefinition>& aApperance, int32_t aOffset);
    static void UnregisterOffsetOverrides(Core::SharedPtr<EntityState>& aEntityState, uint64_t aHash);

    static void RegisterComponentOverrides(Core::SharedPtr<EntityState>& aEntityState, uint64_t aHash,
                                           Red::Handle<Red::AppearanceDefinition>& aApperance);
    static void RegisterComponentOverrides(Core::SharedPtr<EntityState>& aEntityState, uint64_t aHash,
                                           Red::DynArray<Red::AppearancePartOverrides>& aOverrides);
    static void UnregisterComponentOverrides(Core::SharedPtr<EntityState>& aEntityState, uint64_t aHash);

    static void UpdatePartAttributes(Core::SharedPtr<EntityState>& aEntityState,
                                     Red::Handle<Red::AppearanceDefinition>& aApperance);
    static void UpdatePartAssignments(Core::SharedPtr<EntityState>& aEntityState,
                                      Red::DynArray<Red::Handle<Red::IComponent>>& aComponents,
                                      Red::DynArray<Red::ResourcePath>& aPartResources);
    static void UpdatePartAssignments(Red::DynArray<Red::Handle<Red::IComponent>>& aComponents,
                                      Red::ResourcePath aPartResource);

    static bool PrepareDynamicAppearanceName(Red::WeakHandle<Red::Entity>& aEntity,
                                             Red::ResourceTokenPtr<Red::EntityTemplate>& aTemplateToken,
                                             Red::CName& aAppearanceName);
    static void SelectDynamicAppearance(Core::SharedPtr<EntityState>& aEntityState,
                                        DynamicAppearanceName& aSelector,
                                        Red::EntityTemplate* aResource,
                                        Red::TemplateAppearance*& aAppearance);
    static void SelectDynamicAppearance(Core::SharedPtr<EntityState>& aEntityState,
                                        DynamicAppearanceName& aSelector,
                                        Red::AppearanceResource* aResource,
                                        Red::Handle<Red::AppearanceDefinition>& aDefinition);

    static void ApplyDynamicAppearance(Core::SharedPtr<EntityState>& aEntityState);
    static void ApplyDynamicAppearance(Core::SharedPtr<EntityState>& aEntityState,
                                        Red::DynArray<Red::Handle<Red::IComponent>>& aComponents);
    static void ApplyComponentOverrides(Core::SharedPtr<EntityState>& aEntityState, bool aForceUpdate);
    static void ApplyComponentOverrides(Core::SharedPtr<EntityState>& aEntityState,
                                        Red::DynArray<Red::Handle<Red::IComponent>>& aComponents, bool aForceUpdate);
    static void ApplyOffsetOverrides(Core::SharedPtr<EntityState>& aEntityState, Red::DynArray<int32_t>& aOffsets,
                                     Red::DynArray<Red::ResourcePath>& aResourcePaths);

    static void UpdateDynamicAttributes(Core::SharedPtr<EntityState>& aEntityState);
    static void UpdateDynamicAttributes(Core::SharedPtr<EntityState>& aEntityState, Red::TweakDBID aEquippedItemID);
    static void UpdateDynamicAttributes();

    static bool IsUniqueAppearanceName(Red::CName aName);

    static void PatchHeadGarmentOverrides(Core::SharedPtr<EntityState>& aEntityState,
                                          Red::Handle<Red::AppearanceDefinition>& aDefinition);

    static inline Core::UniquePtr<Red::TemplateAppearance> s_emptyAppearance;
    static inline Core::SharedPtr<DynamicAppearanceController> s_dynamicAppearance;
    static inline Core::UniquePtr<OverrideStateManager> s_stateManager;
    static inline Core::SharedPtr<OverrideTagManager> s_tagManager;
    static inline bool s_garmentOffsetsEnabled;
    static inline Red::SharedSpinLock s_mutex;
};
}
