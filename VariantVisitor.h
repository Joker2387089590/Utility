#pragma once
#include <variant>
#ifndef VARIANTS_USE_EXPECTED
#	if __cpp_lib_expected >= 202202L
#		include <expected>
#		define VARIANTS_USE_EXPECTED 1
#	elif __has_include(<boost/outcome.hpp>)
#		include <boost/outcome.hpp>
#		define VARIANTS_USE_EXPECTED 1
#	else
#		define VARIANTS_USE_EXPECTED 0
#	endif
#endif
#include "CallableTrait.h"
#include "Macros.h"

namespace Variants::Detail
{
using Callables::Visitor;

template<typename... Vs>
class VariantImpl : public std::variant<Vs...>
{
	static_assert(sizeof...(Vs) > 0);
	using Base = std::variant<Vs...>;
	using TList = Types::TList<Vs...>;

public:
	using Base::Base;
	DefaultClass(VariantImpl);

	template<typename T> static constexpr auto indexOf = TList::template find<T>();

    static constexpr std::size_t typeCount() { return sizeof...(Vs); }

	template<typename T> constexpr decltype(auto) as()       { return std::get<T>(self()); }
	template<typename T> constexpr decltype(auto) as() const { return std::get<T>(self()); }

	template<typename T> explicit constexpr operator T&() { return as<T>(); }
	template<typename T> explicit constexpr operator const T&() const { return as<T>(); }

	template<typename T> constexpr auto* tryAs() noexcept       { return std::get_if<T>(&self()); }
	template<typename T> constexpr auto* tryAs() const noexcept { return std::get_if<T>(&self()); }

	template<typename... Ts>
	constexpr bool is() const noexcept
	{
		return ((std::holds_alternative<Ts>(self()) || ...) || false);
	}

public: // visit
	template<typename... Fs>
	constexpr decltype(auto) visit(Fs&&... fs)
    {
		return std::visit(Visitor{fwd(fs)...}, self());
	}

	template<typename... Fs>
	constexpr decltype(auto) visit(Fs&&... fs) const
    {
		return std::visit(Visitor{fwd(fs)...}, self());
    }

public: // visitTo
	template<typename T, typename... Fs>
	[[nodiscard]] constexpr decltype(auto) visitTo(Fs&&... fs)
	{
		using Callables::returnAs;
		return std::visit(Visitor{(fwd(fs) | returnAs<T>)...}, self());
	}

	template<typename T, typename... Fs>
	[[nodiscard]] constexpr decltype(auto) visitTo(Fs&&... fs) const
	{
		using Callables::returnAs;
		return std::visit(Visitor{(fwd(fs) | returnAs<T>)...}, self());
	}

public: // select
	template<typename... Ts, typename Fs, typename... Fo>
	constexpr decltype(auto) select(Fs&& selects, Fo&&... others)
	{
		using Callables::argAs;
		return std::visit(Visitor{fwd(selects) | argAs<Ts&>..., fwd(others)...}, self());
	}

	template<typename... Ts, typename Fs, typename... Fo>
	constexpr decltype(auto) select(Fs&& selects, Fo&&... others) const
	{
		using Callables::argAs;
		return std::visit(Visitor{fwd(selects) | argAs<Ts&>..., fwd(others)...}, self());
	}

public: // filter
	// { C<T>::value } -> bool
	template<template<typename> typename C, typename Fs, typename... Fo>
	constexpr decltype(auto) filter(Fs&& selects, Fo&&... others)
	{
		using Tv = typename TList::template RemoveNot<C>;
		return filterHelper(Tv{}, fwd(selects), fwd(others)...);
	}

	template<template<typename> typename C, typename Fs, typename... Fo>
	constexpr decltype(auto) filter(Fs&& selects, Fo&&... others) const
	{
		using Tv = typename TList::template RemoveNot<C>;
		return filterHelper(Tv{}, fwd(selects), fwd(others)...);
	}

private:
	template<typename... Tv, typename Fs, typename... Fo>
	constexpr decltype(auto) filterHelper(Types::TList<Tv...>,
										  Fs&& selects,
										  Fo&&... others)
	{
		using Callables::argAs;
		return std::visit(Visitor{fwd(selects) | argAs<Tv&>..., fwd(others)...}, self());
	}

	template<typename... Tv, typename Fs, typename... Fo>
	constexpr decltype(auto) filterHelper(Types::TList<Tv...>,
										  Fs&& selects,
										  Fo&&... others) const
	{
		using Callables::argAs;
		return std::visit(Visitor{fwd(selects) | argAs<const Tv&>..., fwd(others)...}, self());
	}

public:
	constexpr friend bool operator==(const VariantImpl& a, const VariantImpl& b)
	{
		return a.self() == b.self();
	}
	constexpr friend bool operator!=(const VariantImpl& a, const VariantImpl& b)
	{
		return a.self() != b.self();
	}
	constexpr friend bool operator> (const VariantImpl& a, const VariantImpl& b)
	{
		return a.self() >  b.self();
	}
	constexpr friend bool operator<=(const VariantImpl& a, const VariantImpl& b)
	{
		return a.self() <= b.self();
	}
	constexpr friend bool operator< (const VariantImpl& a, const VariantImpl& b)
	{
		return a.self() <  b.self();
	}
	constexpr friend bool operator>=(const VariantImpl& a, const VariantImpl& b)
	{
		return a.self() >= b.self();
	}

private:
	constexpr Base& self() { return *this; }
	constexpr const Base& self() const { return *this; }
};

template<> class VariantImpl<> { VariantImpl() = delete; };

template<typename... Ts>
using Variant = std::conditional_t<
	sizeof...(Ts) == 0,
	VariantImpl<std::monostate>,
	typename Types::TList<Ts...>::Unique::template Apply<VariantImpl>
>;
} // namespace Variants::Detail

namespace Variants
{
using Detail::Visitor;
using Detail::Variant;
}

#include "MacrosUndef.h"
