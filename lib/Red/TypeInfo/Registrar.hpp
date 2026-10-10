#pragma once

#if defined(__APPLE__)
#include <dlfcn.h>
#include <cstdio>
#include <cstdlib>
#endif

namespace Red
{
class TypeInfoRegistrar
{
public:
    using Callback = void(*)();

    TypeInfoRegistrar(Callback aRegister, Callback aDescribe)
    {
        AddRegisterCallback(aRegister);
        AddDescribeCallback(aDescribe);
    }

    static inline void RegisterDiscovered()
    {
        QueuePendingRegisterCallbacks();
        QueuePendingDescribeCallbacks();
    }

    // Run the queued RedLib callbacks on this thread. macOS calls this from
    // codeware_registerNatives and again from codeware_postLoadScripts.
    static inline void RunPending()
    {
        ProcessPendingRegisterCallbacks();
        ProcessPendingDescriberCallbacks();
    }

    static inline std::size_t PendingRegisterCount()
    {
        return s_registerCallbacks.size();
    }

    static inline std::size_t PendingDescribeCount()
    {
        return s_describeCallbacks.size();
    }

    static inline void AddRegisterCallback(Callback aRegister)
    {
        if (aRegister)
        {
            s_registerCallbacks.push_back(aRegister);
        }
    }

    static inline void AddDescribeCallback(Callback aDescribe)
    {
        if (aDescribe)
        {
            s_describeCallbacks.push_back(aDescribe);
        }
    }

private:
    static inline void QueuePendingRegisterCallbacks()
    {
        if (!s_registerCallbacks.empty())
        {
            CRTTISystem::Get()->AddRegisterCallback(&OnRegister);
        }
    }

    static inline void QueuePendingDescribeCallbacks()
    {
        if (!s_describeCallbacks.empty())
        {
            CRTTISystem::Get()->AddPostRegisterCallback(&OnDescribe);
        }
    }

#if defined(__APPLE__)
    // macOS (R22): with CODEWARE_MAC_DIAG=1 every callback is named before it runs, so a fault inside a registration is located.
    static inline void TraceCallback(const char* aKind, Callback aCallback)
    {
        const char* diag = std::getenv("CODEWARE_MAC_DIAG");
        if (!diag || diag[0] != '1')
            return;
        Dl_info info{};
        const char* sym = (dladdr(reinterpret_cast<const void*>(aCallback), &info) && info.dli_sname) ? info.dli_sname : "?";
        std::fprintf(stderr, "[redlib] %s callback %s\n", aKind, sym);
        std::fflush(stderr);
    }
#else
    static inline void TraceCallback(const char*, Callback) {}
#endif

    static inline void ProcessPendingRegisterCallbacks()
    {
        auto callbacks = std::move(s_registerCallbacks);
        for (const auto& callback :callbacks)
        {
            TraceCallback("register", callback);
            callback();
        }
    }

    static inline void ProcessPendingDescriberCallbacks()
    {
        auto callbacks = std::move(s_describeCallbacks);
        for (const auto& callback :callbacks)
        {
            TraceCallback("describe", callback);
            callback();
        }
    }

    static inline void OnRegister()
    {
        ProcessPendingRegisterCallbacks();
        QueuePendingRegisterCallbacks();
    }

    static inline void OnDescribe()
    {
        ProcessPendingDescriberCallbacks();
        QueuePendingDescribeCallbacks();
    }

    static inline std::vector<Callback> s_registerCallbacks;
    static inline std::vector<Callback> s_describeCallbacks;
};
}
