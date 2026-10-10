#pragma once

namespace Red
{
#if defined(__APPLE__)
// macOS (R22): a result the game returns through x8 whose layout and ownership this code never touches. The user-provided destructor makes the
// type non-trivial for calls, which is what puts it in x8 in clang as in the game; the bytes are forwarded to the caller and never released here.
template<size_t N>
struct SretValue
{
    SretValue() = default;
    SretValue(const SretValue&) = default;
    SretValue& operator=(const SretValue&) = default;
    ~SretValue() {}

    alignas(8) uint8_t data[N]{};
};
#endif

template<typename T>
using ResourceTokenPtr = Red::SharedPtr<Red::ResourceToken<T>>;

template<typename T>
struct Range
{
    constexpr operator bool() const noexcept
    {
        return beginPtr != endPtr;
    }

    [[nodiscard]] inline T* begin() const
    {
        return beginPtr;
    }

    [[nodiscard]] inline T* end() const
    {
        return endPtr;
    }

    [[nodiscard]] auto GetSize() const
    {
        return endPtr - beginPtr;
    }

    [[nodiscard]] bool IsEmpty() const
    {
        return !beginPtr;
    }

    T* beginPtr{}; // 00
    T* endPtr{};   // 08
};
RED4EXT_ASSERT_SIZE(Range<int32_t>, 0x10);
RED4EXT_ASSERT_OFFSET(Range<int32_t>, beginPtr, 0x0);
RED4EXT_ASSERT_OFFSET(Range<int32_t>, endPtr, 0x8);
}
