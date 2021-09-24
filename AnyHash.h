#pragma once
#include <functional> // std::hash
#include <tuple>

/// 小于运算符
#define LESS_TIE(type)																				\
	inline constexpr bool operator< (const type& l, const type& r) { return l.tie() < r.tie(); }	\
	inline constexpr bool operator>=(const type& l, const type& r) { return !(l < r); }

/// 大于运算符
#define GREATER_TIE(type)																			\
	inline constexpr bool operator> (const type& l, const type& r) { return l.tie() > r.tie(); }	\
	inline constexpr bool operator<=(const type& l, const type& r) { return !(l > r); }

/// 等于运算符
#define	EQUAL_TIE(type)																							\
	inline constexpr bool operator==(const type& l, const type& r) { return &l == &r || l.tie() == r.tie(); }	\
	inline constexpr bool operator!=(const type& l, const type& r) { return !(l == r); }

/// 为 type 生成比较运算符，需要 type 有 tie 成员函数
#define COMPARE_TIE(type) LESS_TIE(type) GREATER_TIE(type) EQUAL_TIE(type)

namespace AnyHash
{
	template<typename T>
	constexpr std::size_t combine(std::size_t seed, const T& val)
		noexcept(noexcept(std::hash<T>()(val)))
	{
		return seed ^ (std::hash<T>()(val) + 0x9e3779b9 + (seed << 6) + (seed >> 2));
	}

	// optional auxiliary generic functions to support hash_val() without arguments
	constexpr std::size_t combine(std::size_t seed) noexcept { return seed; }

	// auxiliary generic functions to create a hash value using a seed
	template<typename T, typename... Types>
	constexpr std::size_t combine(std::size_t seed, const T& val, const Types&... args)
		noexcept(noexcept(combine(combine(seed, val), args...)))
	{
		return combine(combine(seed, val), args...);
	}

	// generic function to create a hash value out of a heterogeneous list of arguments
	template<typename... Types>
	constexpr std::size_t calcHash(const Types&... args)
		noexcept(noexcept(combine(0, args...)))
	{
		return combine(0, args...);
	}

	struct TupleHash
	{
		template<typename Tuple>
		constexpr std::size_t operator()(Tuple&& tuple) const
		{
			return std::apply([](auto&&... as) { return calcHash(as...); },
							  std::forward<Tuple>(tuple));
		}
	};

	struct TieHash
	{
		template<typename EnableTie>
		constexpr std::size_t operator()(EnableTie&& tuple) const
		{
			return TupleHash{}(tuple.tie());
		}
	};
}
