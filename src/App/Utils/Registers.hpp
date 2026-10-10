#pragma once

#if defined(_WIN32) || defined(_WIN64)
extern "C"
{
    uint64_t get_r8();
    uint64_t get_r9();
    void set_r8(uint64_t);
    void set_r9(uint64_t);
}
#else
// MAC-NOT-YET: the x64 register helpers (Registers.asm) have no arm64 counterpart. The Journal extension used them to carry r8 and r9
// across its hook bodies; on arm64 the hooked functions take their arguments in x0 to x7, so the Mac hooks declare every argument and
// pass them on (see JournalExtension). These stubs keep the shared sources compiling and return zero.
inline uint64_t get_r8()
{
    return 0;
}
inline uint64_t get_r9()
{
    return 0;
}
inline void set_r8(uint64_t)
{
}
inline void set_r9(uint64_t)
{
}
#endif
