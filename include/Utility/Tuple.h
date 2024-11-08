#pragma once
#include <utility>    // index_sequence
#include <functional> // ref
#include <tuple>
#include "TypeList.h"
#include "Macros.h"

namespace Tuples::Detail
{
using namespace Types;

/// forward declaration

template<typename... Ts> class Tuple;
template<typename... Ts> constexpr auto makeTuple(Ts&&... ts);

/// IndexedType

template<typename T, bool canInherit> class CompressType;

template<typename T>
class CompressType<T, true> : private T
{
public:
	template<typename U>
	constexpr explicit CompressType(U&& d) noexcept(noexcept(T(fwd(d)))) : T(fwd(d)) {}
	DefaultClass(CompressType);

	constexpr auto data() &       noexcept -> T &       { return static_cast<T &>      (*this); };
	constexpr auto data() const&  noexcept -> T const&  { return static_cast<T const&> (*this); };
	constexpr auto data() &&      noexcept -> T &&      { return static_cast<T &&>     (*this); };
	constexpr auto data() const&& noexcept -> T const&& { return static_cast<T const&&>(*this); };
};

template<typename T>
class CompressType<T, false>
{
public:
	template<typename U>
	constexpr explicit CompressType(U&& d) noexcept(noexcept(T(fwd(d)))) : d(fwd(d)) {}
	DefaultClass(CompressType);

	constexpr auto data() &       noexcept -> T &       { return static_cast<T &>      (d); };
	constexpr auto data() const&  noexcept -> T const&  { return static_cast<T const&> (d); };
	constexpr auto data() &&      noexcept -> T &&      { return static_cast<T &&>     (d); };
	constexpr auto data() const&& noexcept -> T const&& { return static_cast<T const&&>(d); };

private:
	T d;
};

// Container of one type of the Tuple,
// use the non-type template argument(std::size_t i) to distinct duplicate types.
template<std::size_t i, typename T>
class IndexedType : public CompressType<T, std::is_class_v<T> && !std::is_final_v<T>>
{
	using Base = CompressType<T, std::is_class_v<T> && !std::is_final_v<T>>;
public:
    using type = T;

    template<typename U>
	constexpr explicit IndexedType(U&& d) noexcept(noexcept(Base(fwd(d)))) : Base(fwd(d)) {}
	using Base::data;
};

/// TypeList

template<typename... Ts>
struct TypeList : Types::TList<Ts...>
{
	template<std::size_t... is> class SizeList;
};

template<typename... Ts>
template<std::size_t... is>
class TypeList<Ts...>::SizeList : private IndexedType<is, Ts>...
{
	static_assert(sizeof...(Ts) == sizeof...(is));
protected: // types access
	template<std::size_t i> using At = typename Types::Detail::AtTrait<i, Ts...>::type;
    template<std::size_t i> using Base = IndexedType<i, At<i>>;

public: // constructor with indexes
	template<typename... Args>
	constexpr SizeList(std::in_place_index_t<0>, Args&&... args)
		noexcept((noexcept(Base<is>(fwd(args))) && ...)) :
		Base<is>(fwd(args))...
	{}

    template<std::size_t i, typename... Args>
	constexpr SizeList(std::in_place_index_t<i>, Args&&... args)
		noexcept(noexcept(SizeList(std::in_place_index<i - 1>, fwd(args)..., At<sizeof...(Ts) - i>{}))) :
		SizeList(std::in_place_index<i - 1>, fwd(args)..., At<sizeof...(Ts) - i>{})
	{}

	DefaultClass(SizeList);

protected:
	template<std::size_t i> constexpr Base<i>* self() { return this; }
	template<std::size_t i> constexpr const Base<i>* self() const { return this; }

public: // container basic functions
	static constexpr std::size_t size() { return TypeList::size(); }

	// get by index
	template<std::size_t i> constexpr decltype(auto) get() &       { return self<i>()->data(); }
	template<std::size_t i> constexpr decltype(auto) get() &&      { return self<i>()->data(); }
	template<std::size_t i> constexpr decltype(auto) get() const&  { return self<i>()->data(); }
	template<std::size_t i> constexpr decltype(auto) get() const&& { return self<i>()->data(); }

	// get by type
	template<typename T> constexpr decltype(auto) get() &       { return get<indexOfFirst<T>()>(); }
	template<typename T> constexpr decltype(auto) get() &&      { return get<indexOfFirst<T>()>(); }
	template<typename T> constexpr decltype(auto) get() const&  { return get<indexOfFirst<T>()>(); }
	template<typename T> constexpr decltype(auto) get() const&& { return get<indexOfFirst<T>()>(); }

	template<typename T>
	static constexpr std::size_t indexOfFirst() { return TypeList::template find<T>(); }

protected: // interface for sub class
    template<typename F>
    constexpr auto forEach(F f) const { return forEachHelper(f, self<is>()...); }
    template<typename F>
    constexpr auto forEach(F f) { return forEachHelper(f, self<is>()...); }

	template<typename F>
	constexpr auto forEachReverse(F f) const { return forEachHelper(f, self<size() - 1 - is>()...); }
    template<typename F>
	constexpr auto forEachReverse(F f) { return forEachHelper(f, self<size() - 1 - is>()...); }

    template<typename C>
    constexpr auto filter(C c) { return filterHelper(filterResult, c, self<is>()...); }
    template<typename C>
    constexpr auto filter(C c) const { return filterHelper(filterResult, c, self<is>()...); }

    template<typename F, std::size_t s, std::enable_if_t<s <= size(), int> = 0>
    constexpr auto take(F f) const 
    {
        if constexpr (s == 0)
            return f();
        else if constexpr (s == size())
            return forEach(f);
        else
            return takeHelper(std::make_index_sequence<s>{});
    }

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

private: // take impl
	template<typename F, std::size_t... i>
	constexpr auto takeHelper(F f, std::index_sequence<i...>) { return f(self<i>()->data()...); }
};

template<typename... Ts, std::size_t... i>
auto reduceTypeSize(std::index_sequence<i...>) ->
    typename TypeList<Ts...>::template SizeList<i...>;
template<typename... Ts>
using ReduceTypeSize =
    decltype(reduceTypeSize<Ts...>(std::make_index_sequence<sizeof...(Ts)>{}));

template<typename... T>  struct IsOneTuple : std::false_type {};
template<typename... Ts> struct IsOneTuple<Tuple<Ts...>> : std::true_type {};
template<typename... T>
inline constexpr bool isOneTuple = IsOneTuple<std::decay_t<T>...>::value;

template<typename... Ts>
class Tuple : private ReduceTypeSize<Ts...>
{
    using Base = ReduceTypeSize<Ts...>;
	template<typename... To> friend class Tuple;

public:
	template<template<typename...> typename F>
	using To = Tuple<typename F<Ts>::type...>;

public: // constructor without indexes
	template<typename... Args, std::enable_if_t<!isOneTuple<Args...>, int> = 0>
	constexpr Tuple(Args&&... args)
		noexcept(noexcept(Base(std::in_place_index<sizeof...(Ts) - sizeof...(Args)>, fwd(args)...))) :
		Base(std::in_place_index<sizeof...(Ts) - sizeof...(Args)>, fwd(args)...)
	{}

	constexpr Tuple() : Base(std::in_place_index<sizeof...(Ts)>) {}

	DefaultClass(Tuple);

public:
    using Base::At;             ///< using T = decltype(tuple)::At<i>;
    using Base::get;            ///< auto& elem = tuple.get<i>();
    using Base::indexOfFirst;   ///< constexpr std::size_t i = tuple.indexOfFirst<T>();
    using Base::size;           ///< constexpr std::size_t s = tuple.size();

	/// Tuple(a, b, c) + Tuple(d, e, f) => Tuple(a, b, c, d, e, f)
	template<typename... Us> [[ nodiscard ]]
	constexpr Tuple<Ts..., Us...> append(const Tuple<Us...>& other) const
    {
        auto vThis = [](auto*... ts) {
			return [ts...](auto*... us) {
				return Tuple<Ts..., Us...>(ts->data()..., us->data()...);
            };
        };
        auto tThis = this->forEach(vThis);
        auto vOther = [&tThis](auto*... us) { return tThis(us...); };
        return other.forEach(vOther);
    }

	/// Tuple(T{}, Tx{}, Ty{}, T{}) => Tuple(Tx{}, Ty{})
	template<typename... T> [[ nodiscard ]]
	constexpr auto remove() const
    {
        auto c = [](auto* t) {
            using Ti = typename std::decay_t<decltype(*t)>::type;
            constexpr bool isOneOf = std::disjunction_v<std::is_same<Ti, T>...>;
            return std::bool_constant<!isOneOf>{};
        };
		auto f = [](auto*... good) {
			return Tuple<typename std::decay_t<decltype(*good)>::type...>(good->data()...);
        };
        return Base::filter(c)(f);
    }

	/// Tuple(a, b, c, ...) => Tuple(f(a), f(b), f(c), ...)
	template<typename F> [[ nodiscard ]]
	constexpr auto map(F f) const
	{
		return Base::forEach([f](auto*... ts) constexpr { return makeTuple(f(ts->data())...); });
    }

	/// Tuple(a, b, c) => Tuple(f(c), f(b), f(a))
	template<typename F> [[ nodiscard ]]
	constexpr auto reverseMap(F f) const
	{
		return Base::forEachReverse([f](auto*... ts) constexpr { return makeTuple(f(ts->data())...); });
    }

	/// Tuple(a, b, c) => Tuple(c, b, a)
	constexpr auto reverse() const
	{
		return reverseMap([](auto& d) constexpr { return std::ref(d); });
	}

	template<typename F>
	constexpr decltype(auto) apply(F f) const
	{
		return Base::forEach([f](auto*... ts) constexpr { return f(ts->data()...); });
	}

	/// Tuple<Tx, Ty, Tz> => Tuple<Tx&, Ty&, Tz&>
    constexpr auto tie() noexcept { return this->map([](auto& data) { return std::ref(data); }); }
    constexpr auto tie() const noexcept { return this->map([](auto& data) { return std::cref(data); }); }
};

template<typename... Ts> Tuple(const Tuple<Ts...>&) -> Tuple<Ts...>;
template<typename... Ts> Tuple(Tuple<Ts...>&&) -> Tuple<Ts...>;

template<typename... Ts, std::enable_if_t<!isOneTuple<Ts...>, int> = 0>
explicit Tuple(Ts...) -> Tuple<Ts...>;

// TODO: std
template<typename... Ts> Tuple(std::tuple<Ts...>) -> Tuple<Ts...>;
template<typename T1, typename T2> Tuple(std::pair<T1, T2>) -> Tuple<T1, T2>;

// remove std::reference_wrapper
template <typename T> struct UnwrapRef { using type = T; };
template <typename T> struct UnwrapRef<std::reference_wrapper<T>> { using type = T&; };
template <typename T> using UnwrapDecay = typename UnwrapRef<typename std::decay<T>::type>::type;

template<typename... Ts>
constexpr auto makeTuple(Ts&&... ts) { return Tuple<UnwrapDecay<Ts>...>(fwd(ts)...); }

template<typename T> struct IsTupleTrait : std::false_type {};
template<typename... Ts> struct IsTupleTrait<Tuple<Ts...>> : std::true_type {};
template<typename T> inline constexpr bool isTuple = IsTupleTrait<std::decay_t<T>>::value;

}

// std support
namespace std
{
template<std::size_t i, typename... Ts>
auto get(Tuples::Detail::Tuple<Ts...>& t)
	-> typename decltype(t)::template At<i> &
{
	return t.template get<i>();
}

template<std::size_t i, typename... Ts>
auto get(Tuples::Detail::Tuple<Ts...>&& t)
	-> typename decltype(t)::template At<i> &&
{
	return std::move(t.template get<i>());
}

template<std::size_t i, typename... Ts>
auto get(const Tuples::Detail::Tuple<Ts...>& t)
	-> typename decltype(t)::template At<i> const&
{
	return t.template get<i>();
}

template<std::size_t i, typename... Ts>
auto get(const Tuples::Detail::Tuple<Ts...>&& t)
	-> typename decltype(t)::template At<i> const&&
{
	using R = typename decltype(t)::template At<i> const&&;
	return static_cast<R>(t.template get<i>());
}

template<typename... Ts>
struct tuple_size<Tuples::Detail::Tuple<Ts...>> :
	public integral_constant<size_t, Tuples::Detail::Tuple<Ts...>::size()>
{};

template<std::size_t i, typename... Ts>
struct tuple_element<i, Tuples::Detail::Tuple<Ts...>>
{
	using type = typename Tuples::Detail::Tuple<Ts...>::template At<i>;
};
}

namespace Tuples
{
using Detail::Tuple, Detail::makeTuple, Detail::isTuple;
}

#include "MacrosUndef.h"
