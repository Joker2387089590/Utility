#pragma once
#include <type_traits>
#include <utility>

namespace Types::Detail
{
/// variadic type list
template<typename... Ts> struct TList;

template<typename... Ts> struct Helper {};

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

template<template<typename...> typename C>
struct Not
{
	template<typename T>
	struct Type { static constexpr bool value = !C<T>::value; };
};

/// remove
template<typename T>
struct IsSameTrait
{
	template<typename Tx> using IsSame = std::is_same<T, Tx>;
};

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

#if defined(__cpp_concepts)
template<std::size_t i, typename T, typename... Ts> requires (i <= sizeof...(Ts))
struct AtTrait<i, T, Ts...>
{
	using type = typename AtTrait<i - 1, Ts...>::type;
};
#else
template<std::size_t i, typename T, typename... Ts>
struct AtTrait<i, T, Ts...>
{
	static_assert(i <= sizeof...(Ts), "The index of At is out of range!");
	using type = typename AtTrait<i - 1, Ts...>::type;
};
#endif

template<typename T, typename... Ts>
struct AtTrait<0, T, Ts...> { using type = T; };

/// repeat
template<std::size_t i, typename Ts, typename Ti> struct RepeatTrait;

template<std::size_t i, typename... Ts, typename... Ti>
struct RepeatTrait<i, Helper<Ts...>, Helper<Ti...>>
{
	using type = typename RepeatTrait<i - 1, Helper<Ts..., Ti...>, Helper<Ti...>>::type;
};

template<typename... Ts, typename... Ti>
struct RepeatTrait<0, Helper<Ts...>, Helper<Ti...>>
{
	using type = TList<Ts...>;
};

/// unique
template<typename T, typename Ts> struct UniqueTrait;

template<typename... Ready, typename Now, typename... Remain>
struct UniqueTrait<Helper<Ready...>, Helper<Now, Remain...>>
{
	using BaseType = std::conditional_t<
		std::disjunction_v<std::is_same<Now, Ready>...>,
		UniqueTrait<Helper<Ready...     >, Helper<Remain...>>,
		UniqueTrait<Helper<Ready..., Now>, Helper<Remain...>>>;

	using type = typename BaseType::type;
	static constexpr std::size_t size = BaseType::size;
};

template<typename... Ready>
struct UniqueTrait<Helper<Ready...>, Helper<>>
{
	static constexpr std::size_t size = sizeof...(Ready);
	using type = TList<Ready...>;
};

/// to rows
template<std::size_t c, typename Tused, typename Tend, typename... Ts>
struct ToRowsTrait;

template<std::size_t c, typename Tused, typename Tend, typename Ti, typename... Ts>
struct ToRowsTrait<c, Tused, Tend, Ti, Ts...>
{
	static_assert(c > 0);
	using Tnew = typename Tend::template Append<Ti>;
	using type = std::conditional_t<
		Tnew::size() == c,
		typename ToRowsTrait<c, typename Tused::template Append<Tnew>, TList<>, Ts...>::type,
		typename ToRowsTrait<c, Tused, Tnew, Ts...>::type
	>;
};

template<std::size_t c, typename Tused, typename Tend>
struct ToRowsTrait<c, Tused, Tend>
{
	using type = std::conditional_t<
		Tend::size() == 0,
		Tused,
		typename Tused::template Append<Tend>
	>;
};

/// slice
template<typename TL, std::size_t b, typename Indexes>
struct SliceTrait;

template<typename TL, std::size_t b, std::size_t... is>
struct SliceTrait<TL, b, std::index_sequence<is...>>
{
	using type = typename TL::template Select<(b + is)...>;
};

template<typename TL, std::size_t b, std::size_t e>
struct SliceCheck
{
	static_assert((0 <= b) && (b <= e) && (e <= TL::size()));
	using type = typename SliceTrait<TL, b, std::make_index_sequence<e - b>>::type;
};

/// remove at
template<typename TL, std::size_t... useless>
struct RemoveAtTrait
{
	static_assert(((useless < TL::size()) && ...), "remove an index that out of TList's range!");

	static constexpr auto begin()
	{
		auto ready = std::index_sequence<>{};
		auto remain = std::make_index_sequence<TL::size()>{};
		return indexFilter(ready, remain);
	}

	template<std::size_t i>
	static constexpr bool isUseless = ((i == useless) || ...);

	template<std::size_t... ready, std::size_t i, std::size_t... remain>
	static constexpr auto indexFilter(std::index_sequence<ready...>, std::index_sequence<i, remain...>)
	{
		if constexpr (isUseless<i>)
			return indexFilter(std::index_sequence<ready...   >{}, std::index_sequence<remain...>{});
		else
			return indexFilter(std::index_sequence<ready..., i>{}, std::index_sequence<remain...>{});
	}

	template<std::size_t... ready>
	static constexpr auto indexFilter(std::index_sequence<ready...>, std::index_sequence<>)
	{
		return typename TL::template Select<ready...>{};
	}
};

// extend
template<typename Te, typename... T>
struct ExtendTrait;

template<typename... Te, typename... T>
struct ExtendTrait<TList<Te...>, T...>
{
	using type = TList<T..., Te...>;
};

// find
inline constexpr std::size_t npos = static_cast<std::size_t>(-1);

template<typename T, typename... Ts>
constexpr std::size_t findFirst()
{
	std::size_t i = 0;
	auto test = [&](bool b) { if(!b) ++i; return b; };
	(test(std::is_same_v<Ts, T>) || ...);
	return (i == sizeof...(Ts)) ? npos : i;
}

template<typename T, typename Ti, typename... Ts>
constexpr std::pair<std::size_t, bool> findLast(std::size_t depth, Helper<Ti, Ts...>)
{
	auto [rdepth, found] = findLast<T>(depth + 1, Helper<Ts...>{});
	if(found)
		return {rdepth, found};
	else
		return {depth, std::is_same_v<T, Ti>};
}

template<typename T>
constexpr std::pair<std::size_t, bool> findLast(std::size_t depth, Helper<>)
{
	return {depth, false};
}

template<typename T, typename... Ts>
constexpr std::size_t findLast()
{
	auto [depth, found] = findLast<T>(0, Helper<Ts...>{});
	return found ? depth : npos;
}

template<std::size_t i>
using SizeConstant = std::integral_constant<std::size_t, i>;

template<typename... Ts>
struct TListBase
{
	// TList can not save values
	constexpr TListBase(...) noexcept {}
	constexpr TListBase(const TListBase&) noexcept = default;
	constexpr TListBase& operator=(const TListBase&) & noexcept = default;
	constexpr TListBase(TListBase&&) noexcept = default;
	constexpr TListBase& operator=(TListBase&&) & noexcept = default;
	constexpr ~TListBase() noexcept = default;
	
	static constexpr auto size = SizeConstant<sizeof...(Ts)>{};

	static constexpr auto empty = std::bool_constant<sizeof...(Ts) == 0>{};

	// TList<T0, T1>::Append<T2, T3> => TList<T0, T1, T2, T3>
	template<typename... Tx>
	using Append = TList<Ts..., Tx...>;

	// TList<T0, T1>::Extend<TList<T2, T3>> => TList<T0, T1, T2, T3>
	template<typename TL>
	using Extend = typename ExtendTrait<TL, Ts...>::type;

	// TList<T0, T1>::Prepend<T2, T3> => TList<T2, T3, T0, T1>
	template<typename... Tx>
	using Prepend = TList<Tx..., Ts...>;

	// F: struct { using type = 'Ts'?; } => F<Ts>::type
	template<template<typename...> typename F>
	using To = TList<typename F<Ts>::type...>;

	// F: 'std::tuple' => std::tuple<Ts...>
	template<template<typename...> typename F>
	using Apply = F<Ts...>;

	// TList<T0, T1, T2> => TList<T2, T1, T0>
	using Reverse = typename ReverseTrait<Ts...>::type;

	// TList<Ta, Tb>::Repeat<3> => TList<Ta, Tb, Ta, Tb, Ta, Tb>
	template<std::size_t s>
	using Repeat = typename RepeatTrait<s, Helper<>, Helper<Ts...>>::type;

	// TList<Ta, Ta, Tb, Tc, Tc>::Unique => TList<Ta, Tb, Tc>
	using Unique = typename UniqueTrait<Helper<>, Helper<Ts...>>::type;

	static constexpr bool repeated = UniqueTrait<Helper<>, Helper<Ts...>>::size != sizeof...(Ts);

	// C: if (C<Ts>::value) remove(Ts)
	template<template<typename...> typename C>
	using RemoveIf = typename RemoveIfTrait<C, Ts...>::type;

	// C: if (!C<Ts>::value) remove(Ts)
	template<template<typename...> typename C>
	using RemoveNot = typename RemoveIfTrait<Not<C>::template Type, Ts...>::type;

	// Tx: if(Ts == Tx) remove(Ts)
	template<typename Tx>
	using Remove = RemoveIf<IsSameTrait<Tx>::template IsSame>;

	static constexpr auto npos = Detail::npos;

	// TList<T0, T1, T2, T1, T0>::find<T1> => 1
	// TList<T0, T1, T2, T1, T0>::find<T3> => npos
	template<typename T>
	static constexpr auto find = SizeConstant<Detail::findFirst<T, Ts...>()>{};

	// TList<T0, T1, T2, T1, T0>::find<T1> => 3
	// TList<T0, T1, T2, T1, T0>::find<T3> => npos
	template<typename T>
	static constexpr auto findLast = SizeConstant<Detail::findLast<T, Ts...>()>{};

	template<typename T>
	static constexpr auto contains = std::bool_constant<find<T>() != npos>{};

	// TList<T1, T2, T3, T4>::ToRows<2> => TList<TList<T1, T2>, TList<T3, T4>>
	template<std::size_t column>
	using ToRows = typename ToRowsTrait<column, TList<>, TList<>, Ts...>::type;

	template<template<typename...> typename Is>
	static constexpr bool ifAll = std::conjunction_v<Is<Ts>...>;

	template<template<typename...> typename Is>
	static constexpr bool ifOne = std::disjunction_v<Is<Ts>...>;

	template<typename T, template<typename, typename> typename Is = std::is_same>
	static constexpr bool allIs = std::conjunction_v<Is<T, Ts>...>;

	template<typename T, template<typename, typename> typename Is = std::is_same>
	static constexpr bool anyIs = std::disjunction_v<Is<T, Ts>...>;
};

/// Empty TList
template<>
struct TList<> : TListBase<> { using TListBase<>::TListBase; };

/// TList
template<typename... Ts>
struct TList : TListBase<Ts...>
{
	using TListBase<Ts...>::TListBase;

	using TListBase<Ts...>::size;

	// TList<T0, T1, ..., Ti, ...> => Ti
	template<std::size_t i>
	using At = typename AtTrait<i, Ts...>::type;

	using First = At<0>;

	using Last = At<sizeof...(Ts) - 1>;

	// TList<T0, T1, T2, T3>::Select<0, 2> => TList<T0, T2>
	template<std::size_t... is>
	using Select = TList<At<is>...>;

	// TList<T0, T1, T2, T3>::Slice<1, 3> => TList<T1, T2, T3>
	template<std::size_t b, std::size_t e = sizeof...(Ts)>
	using Slice = typename SliceCheck<TList, b, e>::type;

	using RemoveFirst = typename RemoveFirstTrait<Ts...>::type;

	using RemoveLast = typename RemoveLastTrait<Ts...>::type;

	template<std::size_t... rs>
	using RemoveAt = decltype(RemoveAtTrait<TList, rs...>::begin());
};

template<typename... Ts> TList(Ts...) -> TList<Ts...>;

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

template<typename... Ts>
using Merge = typename MergeTrait<Ts...>::type;

/// from
template<template<typename...> typename T, typename... Ts>
auto fromTrait(T<Ts...>*) -> TList<Ts...>;

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

template<typename... TLs>
struct ZipTraitHelper<TList<TLs...>>
{
	template<typename TL, typename T1>
	using Prepend = typename TL::template Prepend<T1>;

	template<typename... T1>
	using type = TList<Prepend<TLs, T1>...>;
};

template<typename... T1, typename... TLs>
struct ZipTrait<TList<T1...>, TLs...>
{
	using BaseType = typename ZipTrait<TLs...>::type;
	static_assert(sizeof...(T1) == BaseType::size(),
				  "Zip requires all input TLists have the same size!");
	using type = typename ZipTraitHelper<BaseType>::template type<T1...>;
};

template<typename... Ts>
using Zip = typename ZipTrait<Ts...>::type;

/// unite
template<typename Ready, typename... Lists>
struct UniteTrait;

template<typename... Ready, typename... Now, typename... Remain>
struct UniteTrait<TList<Ready...>, TList<Now...>, Remain...>
{
	using type = typename UniteTrait<typename TList<Ready..., Now...>::Unique, Remain...>::type;
};

template<typename... Ready>
struct UniteTrait<TList<Ready...>>
{
	using type = TList<Ready...>;
};

template<typename... Lists>
using Unite = typename UniteTrait<Lists...>::type;

template<auto v>
struct NonType
{
	static constexpr auto value = v;
	using type = decltype(v);
};

template<auto v>
inline constexpr auto nonType = NonType<v>{};

template<typename... Vs>
struct NonTypeList 
{
	static_assert(
		(std::is_void_v<std::void_t<decltype(Vs::value)>> && ...),
		"Vs should be a list of types with static member 'value'"
	);

	template<template<auto...> typename F>
	using Apply = F<Vs::value...>;
};

template<typename Values>
struct GetValueTrait;

template<template<auto...> typename F, auto... values>
struct GetValueTrait<F<values...>>
{
	using type = NonTypeList<NonType<values>...>;
};

template<typename F>
using GetValue = typename GetValueTrait<F>::type;
} // namespace Types::Detail

namespace Types
{
using Detail::TList;

/// Merge<T1, TList<T2, T3, T4>, TList<T5, T6>> => TList<T1, T2, T3, T4, T5, T6>
using Detail::Merge;

/// any_template<T1, T2, T3, ...> => TList<T1, T2, T3, ...>
using Detail::From;

/// Zip<TList<T1, T2, T3>, TList<T4, T5, T6>> => TList<TList<T1, T4>, TList<T2, T5>, TList<T3, T6>>
using Detail::Zip;

/// Unite<TList<T1, T2, T3>, TList<T1, T2, T4>> => TList<T1, T2, T3, T4>
using Detail::Unite;

using Detail::NonType;
using Detail::NonTypeList;
using Detail::GetValue;
}
