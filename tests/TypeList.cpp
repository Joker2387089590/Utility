#include "../include/Utility/TypeList.h"
#include "common.h"

template<template<std::size_t> typename At, std::size_t N, typename = void>
struct HasAtN : std::false_type {};

template<template<std::size_t> typename At, std::size_t N>
struct HasAtN<At, N, std::void_t<At<N>>> : std::true_type {};

int main()
{
    using namespace Types;

    constexpr auto empty = TList();
    using Empty = std::remove_const_t<decltype(empty)>;
    static_assert(std::is_same_v<Empty, TList<>>);
    static_assert(Empty::size == 0);

    using L1 = Empty::Append<int>;
    static_assert(L1::size == 1);
    static_assert(std::is_same_v<TList<int>, L1>);
    
    using L2 = L1::Extend<TList<char, double>>;
    static_assert(L2::size == 3);
    static_assert(std::is_same_v<TList<int, char, double>, L2>);

    using L3 = L2::Prepend<bool, short>;
    static_assert(L3::size == 5);
    static_assert(std::is_same_v<TList<bool, short, int, char, double>, L3>);

    using L4 = L3::To<std::add_pointer>;
    static_assert(L4::size == 5);
    static_assert(std::is_same_v<TList<bool*, short*, int*, char*, double*>, L4>);

    using L5 = L4::Apply<std::tuple>;
    static_assert(std::is_same_v<std::tuple<bool*, short*, int*, char*, double*>, L5>);

    using L6 = L3::Reverse;
    static_assert(L6::size == 5);
    static_assert(std::is_same_v<TList<double, char, int, short, bool>, L6>);

    using L7 = L2::Repeat<3>;
    static_assert(L7::size == 9);
    static_assert(std::is_same_v<TList<int, char, double, int, char, double, int, char, double>, L7>);
    static_assert(L7::repeated);

    using L8 = L7::Unique;
    static_assert(L8::size == 3);
    static_assert(std::is_same_v<TList<int, char, double>, L8>);
    static_assert(!L8::repeated);

    using L14 = L3::RemoveIf<std::is_integral>;
    static_assert(L14::size == 1);
    static_assert(std::is_same_v<TList<double>, L14>);

    using L15 = L3::RemoveNot<std::is_integral>;
    static_assert(L15::size == 4);
    static_assert(std::is_same_v<TList<bool, short, int, char>, L15>);

    using L16 = L7::Remove<int>;
    static_assert(L16::size == 6);
    static_assert(std::is_same_v<TList<char, double, char, double, char, double>, L16>);

    constexpr std::size_t indexFind = L3::find<int>;
    static_assert(indexFind == 2);

    constexpr std::size_t indexFindLast = L3::findLast<int>;
    static_assert(indexFindLast == 2);

    static_assert(L3::contains<int>);
    static_assert(!L3::contains<float>);

    using L17 = L7::ToRows<3>;
    static_assert(L17::size == 3);
    static_assert(std::is_same_v<TList<TList<int, char, double>, TList<int, char, double>, TList<int, char, double>>, L17>);

    using L18 = L7::ToRows<7>;
    static_assert(L18::size == 2);
    static_assert(std::is_same_v<TList<TList<int, char, double, int, char, double, int>, TList<char, double>>, L18>);

    static_assert(!L3::ifAll<std::is_integral>);
    static_assert(L3::ifAll<std::is_arithmetic>);

    static_assert(L3::ifOne<std::is_integral>);
    static_assert(!L3::ifOne<std::is_class>);

    static_assert(L3::allIs<double, std::is_convertible>);
    static_assert(L3::anyIs<bool>);

    static_assert(std::is_same_v<L3::At<0>, bool>);
    static_assert(std::is_same_v<L3::At<1>, short>);
    static_assert(std::is_same_v<L3::At<2>, int>);
    static_assert(std::is_same_v<L3::At<3>, char>);
    static_assert(std::is_same_v<L3::At<4>, double>);
#if defined(__cpp_concepts)
    static_assert(!HasAtN<L3::At, 5>::value);
#endif

    using L9 = L3::Select<0, 2, 4>;
    static_assert(L9::size == 3);
    static_assert(std::is_same_v<TList<bool, int, double>, L9>);

    using L10 = L3::Slice<1, 4>;
    static_assert(L10::size == 3);
    static_assert(std::is_same_v<TList<short, int, char>, L10>);

    using L11 = L3::RemoveFirst;
    static_assert(L11::size == 4);
    static_assert(std::is_same_v<TList<short, int, char, double>, L11>);

    using L12 = L3::RemoveLast;
    static_assert(L11::size == 4);
    static_assert(std::is_same_v<TList<bool, short, int, char>, L12>);

    using L13 = L3::RemoveAt<0, 2>;
    static_assert(L13::size == 3);
    static_assert(std::is_same_v<TList<short, char, double>, L13>);

    constexpr auto listFromArgs = TList(0, 1u, 3.14f, 2.718, 'a', "hello");
    using ListFromArgs = std::remove_const_t<decltype(listFromArgs)>;
    static_assert(std::is_same_v<ListFromArgs, TList<int, unsigned, float, double, char, const char*>>);

    using L19 = Merge<void, L1, L2>;
    static_assert(L19::size == 1 + L1::size + L2::size);
    static_assert(std::is_same_v<TList<void, int, int, char, double>, L19>);

    using L20 = From<L5>;
    static_assert(L20::size == 5);
    static_assert(std::is_same_v<L4, L20>);

    using L21 = Zip<L4, L3>;
    static_assert(L21::size == 5);
    static_assert(std::is_same_v<TList<TList<bool*, bool>, TList<short*, short>, TList<int*, int>, TList<char*, char>, TList<double*, double>>, L21>);

    using L22 = Unite<Empty, L1, L2>;
    static_assert(std::is_same_v<TList<int, char, double>, L22>);
}
