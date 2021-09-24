#pragma once
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
