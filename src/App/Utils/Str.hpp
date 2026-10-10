#pragma once

namespace App::Str
{
inline std::wstring Widen(const std::string& aStr)
{
    if (aStr.empty())
        return L"";

#if defined(__APPLE__)
    // macOS: wchar_t is 32 bits, so the widened string is the sequence of UTF-8 code points (invalid bytes map to U+FFFD).
    std::wstring result;
    result.reserve(aStr.size());

    for (size_t i = 0; i < aStr.size();)
    {
        const auto lead = static_cast<unsigned char>(aStr[i]);
        uint32_t codePoint = 0xFFFD;
        size_t length = 1;

        if (lead < 0x80)
        {
            codePoint = lead;
        }
        else if ((lead & 0xE0) == 0xC0)
        {
            length = 2;
            codePoint = lead & 0x1F;
        }
        else if ((lead & 0xF0) == 0xE0)
        {
            length = 3;
            codePoint = lead & 0x0F;
        }
        else if ((lead & 0xF8) == 0xF0)
        {
            length = 4;
            codePoint = lead & 0x07;
        }

        if (length > 1)
        {
            if (i + length > aStr.size())
            {
                codePoint = 0xFFFD;
                length = aStr.size() - i;
            }
            else
            {
                for (size_t k = 1; k < length; ++k)
                {
                    const auto next = static_cast<unsigned char>(aStr[i + k]);
                    if ((next & 0xC0) != 0x80)
                    {
                        codePoint = 0xFFFD;
                        length = k;
                        break;
                    }
                    codePoint = (codePoint << 6) | (next & 0x3F);
                }
            }
        }

        result.push_back(static_cast<wchar_t>(codePoint));
        i += length;
    }

    return result;
#else
    auto length = MultiByteToWideChar(CP_UTF8, 0, aStr.data(), static_cast<int32_t>(aStr.size()), nullptr, 0);

    if (length <= 0)
        return L"";

    std::wstring result(length, 0);
    length = MultiByteToWideChar(CP_UTF8, 0, aStr.data(), static_cast<int32_t>(aStr.size()), result.data(),
                                 static_cast<int32_t>(result.size()));

    if (length <= 0)
        return L"";

    return result;
#endif
}

inline std::string SnakeCase(const std::string& aStr)
{
    std::string result;
    bool split = false;

    for (char c : aStr)
    {
        if (isupper(c))
        {
            if (split)
            {
                result += '_';
                split = false;
            }

            result += static_cast<char>(std::tolower(c));
        }
        else
        {
            result += c;
            split = (c != '_');
        }
    }

    return result;
}
}
