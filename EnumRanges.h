#pragma once

template<typename T>
struct EnumRanges
{
	struct Pointer
	{
		int e;
		constexpr auto operator*() const { return static_cast<T>(e); }
		constexpr void operator++() { ++e; }
		constexpr bool operator!=(const Pointer& other) const { return e != other.e; }
	};

	constexpr Pointer begin() const { return {int(T::Begin) + 1}; }
	constexpr Pointer end() const { return {int(T::End)}; }
};

#define DeclEnum(EnumName, ...) enum EnumName { Begin = -1, __VA_ARGS__, End, Size = End }
