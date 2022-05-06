#pragma once
#include <functional> // std::ref
#include <tuple>

namespace Tuples::Detail
{
// forward declaration
template<typename... Ts> class Tuple;
template<typename... Ts> constexpr auto makeTuple(Ts&&... ts);

/// At

template<std::size_t i, typename... Ts> struct AtHelper;

template<std::size_t i, typename T, typename... Ts>
struct AtHelper<i, T, Ts...> { using type = typename AtHelper<i - 1, Ts...>::type; };

template<typename T, typename... Ts>
struct AtHelper<0, T, Ts...> { using type = T; };

template<std::size_t i> struct AtHelper<i> {};

/// Array to Tuple

template<typename... Ts> struct TList {};

template<template<typename...> typename Tu, std::size_t i, typename T, typename Ts>
struct ArrayToTupleTrait;

template<template<typename...> typename Tu, std::size_t i, typename T, typename... Ts>
struct ArrayToTupleTrait<Tu, i, T, TList<Ts...>>
{
    using type = typename ArrayToTupleTrait<Tu, i - 1, T, TList<Ts..., T>>::type;
};

template<template<typename...> typename Tu, typename T, typename... Ts>
struct ArrayToTupleTrait<Tu, 0, T, TList<Ts...>>
{
    using type = Tu<Ts...>;
};

template<template<typename...> typename Tu, typename T, std::size_t N>
using ArrayToTuple = typename ArrayToTupleTrait<Tu, N, T, TList<>>::type;

/// TypeList

/// Container of one type of the Tuple,
/// the non-type template argument(std::size_t) is used for distincting duplicate types.
template<std::size_t i, typename T>
struct IndexedType
{
    using type = T;

    template<typename U>
    explicit constexpr IndexedType(U&& d) noexcept(noexcept(T(std::forward<U>(d)))) :
        data(std::forward<U>(d)) {}

    T data;
};

template<typename... Ts>
struct TypeList { template<std::size_t... is> class SizeList; };

template<typename... Ts>
template<std::size_t... is>
class TypeList<Ts...>::SizeList : private IndexedType<is, Ts>...
{
protected:
    template<std::size_t i> using At = typename AtHelper<i, Ts...>::type;
    template<std::size_t i> using Base = IndexedType<i, At<i>>;

protected:
    template<std::size_t i, typename... Args>
    constexpr SizeList(std::in_place_index_t<i>, Args&&... args)
        noexcept(noexcept(SizeList(std::in_place_index<i - 1>,
                                   std::forward<Args>(args)..., At<sizeof...(Ts) - i>{}))) :
        SizeList(std::in_place_index<i - 1>,
                 std::forward<Args>(args)..., At<sizeof...(Ts) - i>{})
    {}

    template<typename... Args>
    constexpr SizeList(std::in_place_index_t<0>, Args&&... args)
        noexcept((noexcept(Base<is>(std::forward<Args>(args))) && ...)) :
        Base<is>(std::forward<Args>(args))...
    {}

public:
    template<typename... Args>
    constexpr SizeList(Args&&... args)
        noexcept(noexcept(SizeList(std::in_place_index<sizeof...(Ts) - sizeof...(Args)>,
                                   std::forward<Args>(args)...))) :
        SizeList(std::in_place_index<sizeof...(Ts) - sizeof...(Args)>,
                 std::forward<Args>(args)...)
    {}

    constexpr SizeList(SizeList&&) = default;
    constexpr SizeList& operator=(SizeList&&) = default;
    constexpr SizeList(const SizeList&) = default;
    constexpr SizeList& operator=(const SizeList&) = default;
    ~SizeList() = default;

public: // container basic functions
    template<typename T>
    static constexpr std::size_t indexOfFirst()
    {
        std::size_t i = 0;
        auto test = [&](bool b) { if(!b) ++i; return b; };
        (test(std::is_same_v<At<is>, T>) || ...);
        return i;
    }

    template<std::size_t i>
    constexpr const auto& get() const { return self<i>()->data; }

    template<typename T>
    constexpr const auto& get() const { return get<indexOfFirst<T>()>(); }

    template<std::size_t i>
    constexpr auto& get() { return self<i>()->data; }

    template<typename T>
    constexpr auto& get() { return get<indexOfFirst<T>()>(); }

    static constexpr std::size_t size() { return sizeof...(Ts); }

protected:
    template<std::size_t i> constexpr Base<i>* self() { return this; }
    template<std::size_t i> constexpr const Base<i>* self() const { return this; }

protected: // interface for sub class
    template<typename F>
    constexpr auto forEach(F f) const { return forEachHelper(f, self<is>()...); }
    template<typename F>
    constexpr auto forEach(F f) { return forEachHelper(f, self<is>()...); }

    template<typename C>
    constexpr auto filter(C c) { return filterHelper(filterResult, c, self<is>()...); }
    template<typename C>
    constexpr auto filter(C c) const { return filterHelper(filterResult, c, self<is>()...); }

private: // forEach impl
    template<typename F, typename T, typename... Ti>
    constexpr auto forEachHelper(F f, T* t, Ti*... ts) const
    {
        return forEachHelper([f, t](auto... ends) constexpr { return f(t, ends...); }, ts...);
    }
    template<typename F>
    constexpr auto forEachHelper(F f) const { return f(); }

    template<typename F, typename T, typename... Ti>
    constexpr auto forEachHelper(F f, T* t, Ti*... ts)
    {
        return forEachHelper([f, t](auto... ends) constexpr { return f(t, ends...); }, ts...);
    }
    template<typename F>
    constexpr auto forEachHelper(F f) { return f(); }

private: // filter impl
    inline static constexpr auto filterResult = [](auto... good) constexpr
    {
        return [good...](auto&& p) constexpr { return p(good...); };
    };

    template<typename F, typename C, typename T, typename... Ti>
    constexpr auto filterHelper(F f, C c, T* t, Ti*... ts)
    {
        using Condition = decltype(c(t));
        if constexpr (Condition::value)
            return filterHelper([f, t](auto... ends) constexpr { return f(t, ends...); }, c, ts...);
        else
            return filterHelper(f, c, ts...);
    }
    template<typename F, typename C>
    constexpr auto filterHelper(F f, C) { return f(); }

    template<typename F, typename C, typename T, typename... Ti>
    constexpr auto filterHelper(F f, C c, T* t, Ti*... ts) const
    {
        using Condition = decltype(c(t));
        if constexpr (Condition::value)
            return filterHelper([f, t](auto... ends) constexpr { return f(t, ends...); }, c, ts...);
        else
            return filterHelper(f, c, ts...);
    }
    template<typename F, typename C>
    constexpr auto filterHelper(F f, C) const { return f(); }
};

template<typename... Ts, std::size_t... i>
auto reduceTypeSize(std::index_sequence<i...>) ->
    typename TypeList<Ts...>::template SizeList<i...>;
template<typename... Ts>
using ReduceTypeSize =
    decltype(reduceTypeSize<Ts...>(std::make_index_sequence<sizeof...(Ts)>{}));


template<typename... Ts>
class Tuple : private ReduceTypeSize<Ts...>
{
    using Base = ReduceTypeSize<Ts...>;
    template<typename... To> friend class Tuple;
public:
    using Base::Base;

    template<typename... Args>
    constexpr Tuple(Args&&... args) : Base(std::forward<Args>(args)...) {}

    constexpr Tuple() : Base() {}

    // TODO: Tuple(std::tuple<...>), Tuple(std::array<T, N>), Tuple(std::pair<...>)

public:
    using Base::At;
    using Base::get;
    using Base::indexOfFirst;
    using Base::size;

    template<typename... Us>
    constexpr Tuple<Ts..., Us...> append(const Tuple<Us...>& other) const
    {
        auto vThis = [](auto*... ts) {
            return [ts...](auto*... us) {
                return Tuple<Ts..., Us...>(ts->data..., us->data...);
            };
        };
        auto tThis = this->forEach(vThis);
        auto vOther = [&tThis](auto*... us) { return tThis(us...); };
        return other.forEach(vOther);
    }

    /// Remove each type of Ts... that is included by T...
    template<typename... T>
    constexpr auto remove() const
    {
        auto c = [](auto* t) {
            using Ti = typename std::decay_t<decltype(*t)>::type;
            constexpr bool isOneOf = std::disjunction_v<std::is_same<Ti, T>...>;
            return std::bool_constant<!isOneOf>{};
        };
        auto f = [](auto*... good) {
            return Tuple<typename std::decay_t<decltype(*good)>::type...>(good->data...);
        };
        return Base::filter(c)(f);
    }

    template<typename F>
    constexpr auto map(F f)
    {
        return Base::forEach([f](auto*... ts) constexpr { return makeTuple(f(ts->data)...); });
    }

    template<typename F>
    constexpr auto map(F f) const
    {
        return Base::forEach([f](auto*... ts) constexpr { return makeTuple(f(ts->data)...); });
    }

    constexpr auto tie() noexcept { return this->map([](auto& data) { return std::ref(data); }); }
    constexpr auto tie() const noexcept { return this->map([](auto& data) { return std::cref(data); }); }
};

// TODO: std
template<typename... Ts> Tuple(Ts...) -> Tuple<Ts...>;
template<typename... Ts> Tuple(std::tuple<Ts...>) -> Tuple<Ts...>;
template<typename T1, typename T2> Tuple(std::pair<T1, T2>) -> Tuple<T1, T2>;

template <typename T> struct UnwrapRef { using type = T; };
template <typename T> struct UnwrapRef<std::reference_wrapper<T>> { using type = T&; };
template <typename T>
using UnwrapDecay = typename UnwrapRef<typename std::decay<T>::type>::type;

template<typename... Ts>
constexpr auto makeTuple(Ts&&... ts) { return Tuple<UnwrapDecay<Ts>...>(std::forward<Ts>(ts)...); }
}

namespace Tuples
{
using Detail::Tuple, Detail::makeTuple;
}
