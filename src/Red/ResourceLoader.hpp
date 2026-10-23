#pragma once

namespace Red
{
struct ResourceRequest
{
    ResourcePath path; // 00
    uint64_t unk08{};  // 08
    uint64_t unk10{};  // 10 (2.5: flags, see RED4ext::ResourceRequest)
    uint64_t unk18{};  // 18
    int32_t unk20{-1}; // 20 (2.5: archive handle, 2.31: 18)
    int32_t unk24{};   // 24
    uint64_t unk28{};  // 28
};
RED4EXT_ASSERT_SIZE(ResourceRequest, 0x30);
RED4EXT_ASSERT_OFFSET(ResourceRequest, path, 0x0);
}

namespace Raw::ResourceLoader
{
#if defined(__APPLE__)
// Mac (R22): the token comes back in x8 (the Windows out argument disappears). The request is read through the pointer, so editing
// aRequest.path in a HookBefore still redirects the load.
constexpr auto LoadAsync = Core::RawFunc<
    /* addr = */ Red::AddressLib::ResourceLoader_RequestResource,
    /* type = */ Red::SharedPtr<Red::ResourceToken<>> (*)(Red::ResourceLoader* aLoader,
                                                          Red::ResourceRequest& aRequest)>();

// Mac only: the ByPath entry (path in x1, token in x8) inlines the function above, so it is hooked as well (117 callers against 42).
constexpr auto LoadAsyncByPath = Core::RawFunc<
    /* addr = */ Red::AddressLib::ResourceLoader_IssueLoadingRequestByPath,
    /* type = */ Red::SharedPtr<Red::ResourceToken<>> (*)(Red::ResourceLoader* aLoader,
                                                          Red::ResourcePath aPath)>();
#else
constexpr auto LoadAsync = Core::RawFunc<
    /* addr = */ Red::AddressLib::ResourceLoader_RequestResource,
    /* type = */ void (*)(Red::ResourceLoader* aLoader,
                          Red::SharedPtr<Red::ResourceToken<>>& aToken,
                          Red::ResourceRequest& aRequest)>();
#endif

constexpr auto OnUpdate = Core::RawFunc<
    /* addr = */ Red::AddressLib::ResourceLoader_OnUpdate,
    /* type = */ void (*)(Red::ResourceLoader* aLoader)>();
}
