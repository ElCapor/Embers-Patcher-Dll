#pragma once
#include <algorithm>
#include <cstdint>

namespace SDK
{
    template <size_t N>
    struct StringLiteral
    {
        char Value[N];

        constexpr StringLiteral(const char (&Str)[N])
        {
            std::copy_n(Str, N, Value);
        }

        constexpr const char* c_str() const
        {
            return Value;
        }
    };
}
