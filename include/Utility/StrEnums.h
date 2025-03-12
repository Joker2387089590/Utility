#pragma once
#include "Literal.h"
#include "TypeList.h"

namespace StrEnums::Details
{
using namespace Literals;

template<typename T>
concept IsEnum = std::is_enum_v<T>;

template<IsEnum E, E e, StringLiteral t>
struct Element 
{
    static constexpr E value = e;
    static constexpr auto text = t;
};

template<auto condition, typename Elem, typename... Elems>
consteval auto findElement(Types::TList<Elem, Elems...>)
{
    if constexpr (condition(Elem{}))
        return Elem{};
    else
    {
        static_assert(sizeof...(Elems) != 0, "string not found!");
        return findElement<condition>(Types::TList<Elems...>{});
    }
}

template<IsEnum E, typename... Elems>
struct EnumText
{
    using Elements = Types::TList<Elems...>;

    template<E e, StringLiteral t>
    using Add = EnumText<E, Elems..., Element<E, e, t>>;

    template<StringLiteral t>
    static constexpr E enumOf = 
        findElement<[](auto elem) { return elem.text == t; }>(Elements{}).value;

    template<E e>
    static constexpr auto textOf = 
        findElement<[](auto elem) { return elem.value == e; }>(Elements{}).text;
};

#if 0
inline auto foo()
{
    enum X { A, B, C };
    using T = EnumText<X>
        ::Add<A, "A">
        ::Add<B, "B">
        ::Add<C, "C">;

    static_assert(T::enumOf<"A"> == A);
    static_assert(T::enumOf<"B"> == B);
    static_assert(T::enumOf<"C"> == C);
    static_assert(T::textOf<A> == "A");
    static_assert(T::textOf<B> == "B");
    static_assert(T::textOf<C> == "C");
}
#endif
} // namespace StrEnums::Details

namespace StrEnums
{
using Details::EnumText;
}
