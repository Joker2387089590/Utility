#pragma once
#include <stdexcept>
#include <vector>
#include <array>

namespace DataViews
{
template<typename T>
class FatPtr
{
public:
	using type = T;

	constexpr FatPtr(type* d, std::size_t s) noexcept : data{d}, size{s} {}
	~FatPtr() = default;

	operator bool() const noexcept { return !!data; }

	type* begin() const noexcept { return data; }
	type* end() const noexcept { return data + size; }

	type* begin() noexcept { return data; }
	type* end() noexcept { return data + size; }

public:
	type* data;
	std::size_t size;
};

template<typename T>
class Mut : public FatPtr<T>
{
	using Base = FatPtr<T>;
	using MT = std::remove_const_t<T>;
public:
	using typename Base::type;
	using Base::Base;

	template<std::size_t N>
	constexpr Mut(MT (&arr)[N]) noexcept : Base(arr, N) {}

	template<std::size_t N>
	constexpr Mut(std::array<MT, N>& d) noexcept : Base(d.data(), d.size()) {}

	Mut(std::vector<MT>& d) noexcept : Base(d.data(), d.size()) {}
};

template<typename T>
class Const : public Mut<T>
{
	using Base = Mut<T>;
	using MT = std::remove_const_t<T>;
	using CT = std::add_const_t<T>;
public:
	using typename Base::type;
	using Base::Base;

	template<std::size_t N>
	constexpr Const(CT (&arr)[N]) noexcept : Base(arr, N) {}

	template<std::size_t N>
	constexpr Const(const std::array<MT, N>& d) noexcept : Base(d.data(), d.size()) {}

	Const(const std::vector<MT>& d) noexcept : Base(d.data(), d.size()) {}
};

template<typename T>
using DB = std::conditional_t<std::is_const_v<T>, Const<T>, Mut<T>>;

template<typename T>
class DataView : public DB<T>
{
	using Base = DB<T>;
	using MT = std::remove_const_t<T>;
	using CT = std::add_const_t<T>;
public:
	using typename Base::type;
	using Base::Base;

	constexpr operator DataView<CT>() const noexcept { return {data, size}; }
	using Base::operator bool;

	using Base::begin;
	using Base::end;

public:
	using Base::data;
	using Base::size;
};

template<typename T>
inline bool operator==(DataView<T> l, DataView<T> r)
{
	return std::equal(l.begin(), l.end(), r.begin());
}

template<typename T>
inline bool operator!=(DataView<T> l, DataView<T> r)
{
	return !(l == r);
}

template<typename T, std::size_t N>
DataView(std::array<T, N>&) -> DataView<T>;
template<typename T, std::size_t N>
DataView(const std::array<T, N>&) -> DataView<const T>;
template<typename T>
DataView(std::vector<T>&) -> DataView<T>;
template<typename T>
DataView(const std::vector<T>&) -> DataView<const T>;

template<typename T>
inline constexpr std::size_t sizeOf = sizeof(T);
template<>
inline constexpr std::size_t sizeOf<void> = 1;

template<>
class DataView<const void> : public FatPtr<const void>
{
public:
	template<typename T>
	explicit DataView(DataView<T> view) noexcept :
		FatPtr{view.data, view.size * sizeOf<T>} {}

	template<typename T>
	DataView(T* data, std::size_t size) : FatPtr{data, size * sizeOf<T>} {}

	template<typename T>
	auto cast() const
	{
		using CT = std::add_const_t<T>;
		if(size % sizeOf<CT> != 0)
			throw std::logic_error("Bad DataView cast!");
		return DataView<CT>{ static_cast<CT*>(data), size / sizeOf<CT> };
	}
};

template<>
class DataView<void> : public FatPtr<void>
{
public:
	template<typename T>
	explicit DataView(DataView<T> view) noexcept :
		FatPtr{view.data, view.size * sizeOf<T>} {}

	operator DataView<const void>() noexcept { return {data, size}; }

	template<typename T>
	auto cast() const
	{
		if(size % sizeOf<T> != 0) throw std::logic_error("Bad DataView cast!");
		return DataView<T>{ static_cast<T*>(data), size / sizeOf<T> };
	}
};

using CRawView = DataView<const void>;
using RawView = DataView<void>;
}

using DataViews::DataView;
using DataViews::RawView;
using DataViews::CRawView;
