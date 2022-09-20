#pragma once
#include <type_traits>

namespace Enums::Detail
{
#define DeclEnum(EnumName, ...) enum EnumName { __VA_ARGS__, End, Size = End }

template<typename T, std::enable_if_t<std::is_enum_v<T>, int> = 0>
struct Iterator
{
	using ValueType = std::underlying_type_t<T>;
	constexpr T operator*() const { return static_cast<T>(value); }
	constexpr Iterator& operator++() { ++value; return *this; }
	constexpr bool operator==(const Iterator& other) const { return value == other.value; }
	constexpr bool operator!=(const Iterator& other) const { return !(*this == other); }
	ValueType value;
};

template<typename T>
struct [[deprecated("use MakeEnum instead")]] EnumRanges
{
	using Iterator = Detail::Iterator<T>;
	using ValueType = typename Iterator::ValueType;
	constexpr Iterator begin() const noexcept { return { ValueType(0) }; }
	constexpr Iterator end() const noexcept { return { ValueType(T::End) }; }
};

template<typename E> [[deprecated("use MakeEnum instead")]]
inline constexpr auto enumRanges = EnumRanges<E>{};

#define MakeEnum(EnumName, ...) \
enum class EnumName { __VA_ARGS__, End, Size = End }; \
inline constexpr auto begin(EnumName) noexcept { return Enums::Detail::Iterator<EnumName>{0}; } \
inline constexpr auto end(EnumName) noexcept { return Enums::Detail::Iterator<EnumName>{int(EnumName::End)}; } \

}

using Enums::Detail::enumRanges;
