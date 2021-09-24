#pragma once
#include <type_traits>

#define DeclEnum(EnumName, ...) \
 	enum EnumName { Begin = -1, __VA_ARGS__, End, Size = End }

template<typename T, std::enable_if_t<std::is_enum_v<T>, int> = 0>
struct EnumRanges
{
	using ValueType = std::underlying_type_t<T>;
	struct Pointer
	{
		ValueType value;
		constexpr T operator*() const { return static_cast<T>(value); }
		constexpr Pointer& operator++() { ++value; }
		constexpr bool operator==(const Pointer& other) const { return value == other.value; }
		constexpr bool operator!=(const Pointer& other) const { return !(*this == other); }
	};

	constexpr Pointer begin() const { return {int(T::Begin) + 1}; }
	constexpr Pointer end() const { return {int(T::End)}; }
};

template<typename E>
inline constexpr auto enumRanges = EnumRanges<E>{};
