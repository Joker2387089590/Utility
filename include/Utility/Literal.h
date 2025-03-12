#pragma once
#include <algorithm> // std::copy_n
#include <string_view>

namespace Literals
{
template<typename Char, std::size_t N>
struct StringLiteral
{
public:
    using StdStringView = std::basic_string_view<Char>;

    constexpr StringLiteral(const Char (&str)[N]) { std::copy_n(str, N, value); }
    constexpr ~StringLiteral() = default;

    constexpr operator StdStringView() const { return {value}; }

    constexpr bool operator==(StdStringView v) const noexcept
    {
        return StdStringView(*this) == v;
    }

#if __cplusplus >= 202002L
    friend constexpr auto operator<=>(StringLiteral l, StdStringView v) noexcept
    {
        return StdStringView(l) <=> v;
    }
#else
    // maybe someone can complete this...
#endif

public:
    Char value[N];
};

template<typename Char, std::size_t N>
StringLiteral(const Char (&str)[N]) -> StringLiteral<Char, N>;

template<typename C, typename T>
struct IsStringLiteral : std::false_type {};

template<typename Char, std::size_t N>
struct IsStringLiteral<Char, StringLiteral<Char, N>> : std::true_type {};

template<typename T, typename Char, std::size_t N>
constexpr bool operator==(const T& v, const StringLiteral<Char, N>& l) noexcept
{
    using StdStringView = std::basic_string_view<Char>;
    return StdStringView(v) == StdStringView(l); 
}
} // namespace Literals
