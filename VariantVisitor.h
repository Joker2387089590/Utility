#pragma once
#include <variant>
#include "CallableTrait.h"

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

	template<typename... Fs>
	constexpr decltype(auto) visit(Fs&&... fs)
    {
        return std::visit(Visitor{std::forward<Fs>(fs)...}, self());
	}

	template<typename... Fs>
	constexpr decltype(auto) visit(Fs&&... fs) const
    {
        return std::visit(Visitor{std::forward<Fs>(fs)...}, self());
    }

	template<typename T, typename... Fs>
	constexpr decltype(auto) visitTo(Fs&&... fs)
	{
		using Callables::returnAs;
		return std::visit(Visitor{(std::forward<Fs>(fs) | returnAs<T>)...}, self());
	}

	template<typename T, typename... Fs>
	constexpr decltype(auto) visitTo(Fs&&... fs) const
	{
		using Callables::returnAs;
		return std::visit(Visitor{(std::forward<Fs>(fs) | returnAs<T>)...}, self());
	}

private:
    Base& self() { return *this; }
    const Base& self() const { return *this; }
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
