#pragma once
#include <algorithm> // std::copy_n
#include <string_view>

namespace Literals
{
template<typename C, std::size_t N>
struct StringLiteral;

template<typename T>
struct IsStringLiteralTrait : std::false_type {};

template<typename C, std::size_t N>
struct IsStringLiteralTrait<StringLiteral<C, N>> : std::true_type {};

template<auto literal>
constexpr bool isStringLiteral = IsStringLiteralTrait<decltype(literal)>::value;

#ifdef __cpp_concepts
template<auto literal>
concept IsStringLiteral = IsStringLiteralTrait<decltype(literal)>::value;
#endif

template<typename C, std::size_t N>
struct StringLiteral
{
public:
    static_assert(N >= 1, "StringLiteral must be compatible to a null-terminated string.");
	using Char = C;
    using StdStringView = std::basic_string_view<Char>;

    constexpr StringLiteral() : value{} {}
    constexpr StringLiteral(const Char (&str)[N]) : value{} { std::copy_n(str, N, value); }
    explicit constexpr StringLiteral(const Char* str) : value{} { std::copy_n(str, N, value); }

    constexpr StringLiteral(const StringLiteral&) noexcept = default;
    constexpr StringLiteral& operator=(const StringLiteral&) & noexcept = default;
    
    constexpr StringLiteral(StringLiteral&&) noexcept = default;
    constexpr StringLiteral& operator=(StringLiteral&&) & noexcept = default;

#if __cplusplus >= 202002L
    constexpr 
#endif
        ~StringLiteral() noexcept = default;

public:
	constexpr const Char* data() const { return value; }
	static constexpr std::size_t size() { return N - 1; }
	constexpr operator StdStringView() const { return {data(), size()}; }

public:
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

#if __cplusplus >= 202002L
public:
    template<auto prefix>
    constexpr auto removePrefix() const
    {
        static_assert(isStringLiteral<prefix>);
        using Cp = typename decltype(prefix)::Char;
        constexpr auto Np = prefix.size();
        static_assert(N >= Np, "input must be longer than prefix");
        static_assert(std::is_same_v<Cp, Char>, "input and prefix must be same type");

        if constexpr (StdStringView(*this).starts_with(StdStringView(prefix)))
        {
            using Result = StringLiteral<Char, N - Np + 1>;
            return Result(data() + prefix.size());
        }
        else
        {
            return *this;
        }
    }
#endif

public:
    Char value[N];
};

template<typename Char, std::size_t N>
StringLiteral(const Char (&str)[N]) -> StringLiteral<Char, N>;

template<typename T, typename Char, std::size_t N>
constexpr bool operator==(const T& v, const StringLiteral<Char, N>& l) noexcept
{
    using StdStringView = std::basic_string_view<Char>;
    return StdStringView(v) == StdStringView(l); 
}
} // namespace Literals
