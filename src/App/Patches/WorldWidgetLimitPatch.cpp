#include "WorldWidgetLimitPatch.hpp"
#if defined(__APPLE__)
#include <libkern/OSCacheControl.h>
#include <mach/mach.h>
#endif

void App::WorldWidgetLimitPatch::OnBootstrap()
{
#if defined(_WIN32) || defined(_WIN64)
    auto& code = Core::RawPtr<Red::AddressLib::InkWorldLayer_UpdateComponents, uint8_t[0x100]>::Ref();
    uint8_t op[] = {0x83, 0xFB, 0x0A, 0x73, 0x16};
    bool success = false;

    DWORD oldProtect = 0;
    VirtualProtect(code, sizeof(code), PAGE_EXECUTE_WRITECOPY, &oldProtect);

    auto it = std::search(std::begin(code), std::end(code), std::begin(op), std::end(op));
    if (it != std::end(code))
    {
        *(it + 3) = 0x90;
        *(it + 4) = 0x90;
        success = true;
    }

    VirtualProtect(code, sizeof(code), oldProtect, nullptr);

    if (success)
    {
        LogInfo("WorldWidgetComponent limit patch is successfully applied.");
    }
    else
    {
        LogWarning("WorldWidgetComponent limit patch is not applied, expected pattern was not found.");
    }
#elif defined(__APPLE__)
    // macOS (R22, /tmp/axl-d-signatures.md section 3): the world widget update lambda is inlined into the parallel-for job body 0x491DDB0, where the
    // limit is `cmp x22,#9 ; b.hi over_limit` (job body + 0x12C / +0x130). The branch is replaced by a nop, as the Windows patch nops the jae. The
    // instruction pair is searched in the first 0x200 bytes (the Windows window of 0x100 does not reach it) and must match exactly once.
    constexpr uint32_t CmpMask = 0xFFFFFC1Fu;
    constexpr uint32_t CmpValue = 0xF100241Fu;  // cmp xN, #9
    constexpr uint32_t BhiMask = 0xFF00001Fu;
    constexpr uint32_t BhiValue = 0x54000008u;  // b.hi <imm19>
    constexpr uint32_t Nop = 0xD503201Fu;
    constexpr size_t WindowWords = 0x200 / sizeof(uint32_t);

    const auto address = Core::RawPtr<Red::AddressLib::InkWorldLayer_UpdateComponents, uint8_t>::GetAddress();
    auto* code = reinterpret_cast<uint32_t*>(address);

    if (!code)
    {
        LogWarning("WorldWidgetComponent limit patch is not applied, the job body address is not available.");
        return;
    }

    size_t match = WindowWords;
    size_t matches = 0;
    for (size_t i = 0; i + 1 < WindowWords; ++i)
    {
        if ((code[i] & CmpMask) == CmpValue && (code[i + 1] & BhiMask) == BhiValue)
        {
            match = i + 1;
            ++matches;
        }
    }

    if (matches != 1)
    {
        LogWarning("WorldWidgetComponent limit patch is not applied, expected pattern was not found (matches: {}).", matches);
        return;
    }

    auto* target = &code[match];
    const auto page = reinterpret_cast<uintptr_t>(target) & ~static_cast<uintptr_t>(vm_page_size - 1);

    bool success = vm_protect(mach_task_self(), static_cast<vm_address_t>(page), vm_page_size, FALSE,
                              VM_PROT_READ | VM_PROT_WRITE | VM_PROT_COPY) == KERN_SUCCESS;
    if (success)
    {
        *target = Nop;
        success = vm_protect(mach_task_self(), static_cast<vm_address_t>(page), vm_page_size, FALSE,
                             VM_PROT_READ | VM_PROT_EXECUTE) == KERN_SUCCESS;
        sys_icache_invalidate(target, sizeof(uint32_t));
    }

    if (success && *target == Nop)
    {
        LogInfo("WorldWidgetComponent limit patch is successfully applied.");
    }
    else
    {
        LogWarning("WorldWidgetComponent limit patch is not applied, the code page could not be written.");
    }
#endif
}
