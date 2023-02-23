#pragma once
#include <variant>
#include "CallableTrait.h"
#include "Macros.h"

namespace Variants::Detail
{
using Callables::Visitor;

template<typename... Vs>
class UniqueVariant : public std::variant<Vs...>
{
	using Base = std::variant<Vs...>;
public:
	using Base::Base;

    static constexpr std::size_t typeCount() { return sizeof...(Vs); }

	template<typename T> constexpr decltype(auto) as()       { return std::get<T>(self()); }
	template<typename T> constexpr decltype(auto) as() const { return std::get<T>(self()); }

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
		using Tv = typename Types::TList<Vs...>::template RemoveNot<C>;
		return filterHelper(Tv{}, fwd(selects), fwd(others)...);
	}

	template<template<typename> typename C, typename Fs, typename... Fo>
	constexpr decltype(auto) filter(Fs&& selects, Fo&&... others) const
	{
		using Tv = typename Types::TList<Vs...>::template RemoveNot<C>;
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
	constexpr friend bool operator==(const UniqueVariant& a, const UniqueVariant& b)
	{
		return a.self() == b.self();
	}
	constexpr friend bool operator!=(const UniqueVariant& a, const UniqueVariant& b)
	{
		return a.self() != b.self();
	}
	constexpr friend bool operator> (const UniqueVariant& a, const UniqueVariant& b)
	{
		return a.self() >  b.self();
	}
	constexpr friend bool operator<=(const UniqueVariant& a, const UniqueVariant& b)
	{
		return a.self() <= b.self();
	}
	constexpr friend bool operator< (const UniqueVariant& a, const UniqueVariant& b)
	{
		return a.self() <  b.self();
	}
	constexpr friend bool operator>=(const UniqueVariant& a, const UniqueVariant& b)
	{
		return a.self() >= b.self();
	}

private:
	constexpr Base& self() { return *this; }
	constexpr const Base& self() const { return *this; }
};

template<typename... Ts>
struct TakeOneTrait
{
	using type = typename Types::TList<Ts...>::Unique::template Apply<UniqueVariant>;
};

template<>
struct TakeOneTrait<>
{
	using type = UniqueVariant<std::monostate>;
};

template<typename... Ts> using Variant = typename TakeOneTrait<Ts...>::type;
}

namespace Variants
{
using Detail::Visitor;
using Detail::Variant;
}

#include "MacrosUndef.h"
