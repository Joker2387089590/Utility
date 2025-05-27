#pragma once
#include <optional>
#include <array>
#include "Literal.h"
#include "TypeList.h"

namespace StrEnums::Details
{
using namespace Literals;

template<typename T> concept IsEnum = std::is_enum_v<T>;

template<IsEnum E, typename... Elems> struct EnumText;

template<IsEnum E, typename... Elems>
struct Builder
{
    template<E e, StringLiteral t, auto... values>
    struct Element
    {
        static constexpr E value = e;
        static constexpr auto text = t;
        using Values = Types::NonTypeList<Types::NonType<values>...>;

        static constexpr auto pair = std::pair<E, std::string_view>{value, text};
    };

    // TODO: check text duplicates
    template<E e, StringLiteral t, auto... values>
    using Add = Builder<E, Elems..., Element<e, t, values...>>;

    using Build = EnumText<E, Elems...>;
};

template<IsEnum E, typename... Elems>
struct EnumText
{
    using type = E;
    using Elements = Types::TList<Elems...>;

    static constexpr auto texts = std::array<std::string_view, sizeof...(Elems)>{Elems::text...};

    template<typename If>
    static constexpr void findBy(If&& condition)
    {
        (condition(Elems::value, Elems::text) || ...);
    }

    static constexpr std::optional<E> findEnumOf(std::string_view text)
    {
        std::optional<E> result = std::nullopt;
        findBy([&](auto value, auto t) {
            if(t != text) return false;
            result.emplace(value);
            return true;
        });
        return result;
    }

    template<StringLiteral t>
    static constexpr E enumOf = findEnumOf(t).value();

    static constexpr auto indexOf(E e)
    {
        std::size_t index = 0;
        findBy([&](auto value, auto) {
            if(value == e) return true;
            ++index;
            return false;
        });
        return index;
    }

    static constexpr std::string_view findTextOf(E e)
    {
        auto index = indexOf(e);
        if(index >= sizeof...(Elems)) return {};
        return texts[index];
    }

    template<E e>
    static constexpr StringLiteral textOf = Elements::template At<indexOf(e)>::text;

    static constexpr std::optional<E> from(std::underlying_type_t<E> u)
    {
        if(indexOf(static_cast<E>(u)) >= sizeof...(Elems)) return std::nullopt;
        return static_cast<E>(u);
    }
};
} // namespace StrEnums::Details

namespace StrEnums
{
using Details::EnumText;
using Details::Builder;
}
