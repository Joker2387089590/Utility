#pragma once
#include <type_traits>

namespace Types::Detail
{
template<typename... Ts> struct TList;

/// remove last
template<typename... Ts> struct RemoveLastTrait;

template<typename T, typename... Ts>
struct RemoveLastTrait<T, Ts...>
{
	using type = typename RemoveLastTrait<Ts...>::type::template Prepend<T>;
};

template<typename T>
struct RemoveLastTrait<T> { using type = TList<>; };

/// remove first
template<typename... Ts> struct RemoveFirstTrait;

template<typename T, typename... Ts>
struct RemoveFirstTrait<T, Ts...> { using type = TList<Ts...>; };

/// remove if
template<template<typename...> typename C, typename... Ts> struct RemoveIfTrait;

template<template<typename...> typename C, typename Ti, typename... Ts>
struct RemoveIfTrait<C, Ti, Ts...>
{
	using BaseType = typename RemoveIfTrait<C, Ts...>::type;
	using type = std::conditional_t<
		C<Ti>::value,
		BaseType,
		typename BaseType::template Prepend<Ti>>;
};

template<template<typename...> typename C>
struct RemoveIfTrait<C> { using type = TList<>; };

/// remove
template<typename T>
struct IsSameTrait { template<typename Tx> using IsSame = std::is_same<T, Tx>; };

/// reverse
template<typename... Ts> struct ReverseTrait;

template<typename Ti, typename... Ts>
struct ReverseTrait<Ti, Ts...>
{
	using BaseType = typename ReverseTrait<Ts...>::type;
	using type = typename BaseType::template Append<Ti>;
};

template<>
struct ReverseTrait<> { using type = TList<>; };

/// at
template<std::size_t i, typename... Ts> struct AtTrait;

template<std::size_t i, typename T, typename... Ts>
struct AtTrait<i, T, Ts...> { using type = typename AtTrait<i - 1, Ts...>::type; };

template<typename T, typename... Ts>
struct AtTrait<0, T, Ts...> { using type = T; };

/// TList
template<typename... Ts>
struct TList
{
	// F: struct { using type = 'Ts'?; } => F<Ts>::type
	template<template<typename...> typename F>
	using To = TList<typename F<Ts>::type...>;

	// F: 'std::tuple' => std::tuple<Ts...>
	template<template<typename...> typename F>
	using Apply = F<Ts...>;

	template<typename... Tx>
	using Prepend = TList<Tx..., Ts...>;

	template<typename... Tx>
	using Append = TList<Ts..., Tx...>;

	using RemoveFirst = typename RemoveFirstTrait<Ts...>::type;

	using RemoveLast = typename RemoveLastTrait<Ts...>::type;

	// C: if (C<Ts>::value) remove(Ts)
	template<template<typename...> typename C>
	using RemoveIf = typename RemoveIfTrait<C, Ts...>::type;

	// Tx: if(Ts == Tx) remove(Ts)
	template<typename Tx>
	using Remove = RemoveIf<IsSameTrait<Tx>::template IsSame>;

	// TList<T0, T1, T2> => TList<T2, T1, T0>
	using Reverse = typename ReverseTrait<Ts...>::type;

	// T0, T1, ..., Ti, ... => Ti
	template<std::size_t i>
	using At = typename AtTrait<i, Ts...>::type;

	static constexpr std::size_t size() { return sizeof...(Ts); }

	static constexpr bool empty() { return size() == 0; }

	using First = At<0>;

	using Last = At<size() - 1>;

	static constexpr std::size_t npos = -1;

	// TList<X, 'T', Y, T, Z> => 1
	// TList<X, Y, Z> => npos
	template<typename T>
	static constexpr std::size_t find()
	{
		std::size_t i = 0;
		auto test = [&](bool b) { if(!b) ++i; return b; };
		(test(std::is_same_v<Ts, T>) || ...);
		return i == size() ? npos : i;
	}

	// TList<X, T, Y, 'T', Z> => 3
	// TList<X, Y, Z> => npos
	template<typename T>
	static constexpr std::size_t findLast()
	{
		std::size_t ri = Reverse::template find<T>();
		return ri == npos ? npos : size() - ri - 1;
	}
};

/// Empty TList
template<>
struct TList<>
{
	template<template<typename...> typename F> using To = TList;

	template<template<typename...> typename F> using Apply = F<>;

	template<typename... Tx> using Prepend = TList<Tx...>;

	template<typename... Tx> using Append = TList<Tx...>;

	template<template<typename...> typename> using RemoveIf = TList;

	template<typename> using Remove = TList;

	using Reverse = TList;

	static constexpr std::size_t size() { return 0; }

	static constexpr bool empty() { return true; }

	static constexpr std::size_t npos = -1;

	template<typename>
	static constexpr std::size_t find() { return npos; }

	template<typename>
	static constexpr std::size_t findLast() { return npos; }
};

/// merge
template<typename... Ts> struct MergeTrait;

template<typename T1, typename T2, typename... Ts>
struct MergeTrait<T1, T2, Ts...>
{
	using TwoMerge = typename MergeTrait<T1, T2>::type;
	using type = typename MergeTrait<TwoMerge, Ts...>::type;
};

template<typename T1, typename T2>
struct MergeTrait<T1, T2>
{
	using type = TList<T1, T2>;
};

template<typename T1, typename... T2>
struct MergeTrait<T1, TList<T2...>>
{
	using type = TList<T1, T2...>;
};

template<typename... T1, typename T2>
struct MergeTrait<TList<T1...>, T2>
{
	using type = TList<T1..., T2>;
};

template<typename... T1, typename... T2>
struct MergeTrait<TList<T1...>, TList<T2...>>
{
	using type = TList<T1..., T2...>;
};

// Merge<T1, TList<T2, T3, T4>, TList<T5, T6>> => TList<T1, T2, T3, T4, T5, T6>
template<typename... Ts>
using Merge = typename MergeTrait<Ts...>::type;

/// from
template<template<typename...> typename T, typename... Ts>
auto fromTrait(T<Ts...>*) -> TList<Ts...>;

// std::tuple<T1, T2, T3, ...> => TList<T1, T2, T3, ...>
template<typename T>
using From = decltype(fromTrait(std::add_pointer_t<std::decay_t<T>>{}));

/// zip
template<typename... Ts> struct ZipTrait;

template<>
struct ZipTrait<>
{
	using type = TList<>;
};

template<typename... Ts>
struct ZipTrait<TList<Ts...>>
{
	using type = TList<TList<Ts>...>;
};

template<typename Ts> struct ZipTraitHelper;

template<typename... Ts>
struct ZipTraitHelper<TList<Ts...>>
{
	template<typename... T1>
	using type = TList<Merge<TList<T1>, Ts>...>;
};

template<typename... T1, typename... Ts>
struct ZipTrait<TList<T1...>, Ts...>
{
	using BaseType = typename ZipTrait<Ts...>::type;
	using type = typename ZipTraitHelper<BaseType>::template type<T1...>;
};

template<typename... Ts>
using Zip = typename ZipTrait<Ts...>::type;
}

namespace Types
{
using Detail::TList;
using Detail::Merge;
using Detail::From;
using Detail::Zip;
}
