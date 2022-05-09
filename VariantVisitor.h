#pragma once
#include <variant>
#include "Tuple.h"

namespace Variants::Detail
{
template<typename... Ts> struct TList {};

template<typename F> struct FuncPtrWrapper { using type = F; };

template<typename R, typename... Args>
struct FuncPtrWrapper<R(Args...)>
{
	using FuncPtr = R(*)(Args...);
	struct type
	{
		type(FuncPtr f) noexcept : f(f) {}
		R operator()(Args... args) const { return f(args...); }
		FuncPtr f;
	};
};

template<typename... Vs>
struct Visitor : FuncPtrWrapper<Vs>::type...
{
	template<typename V> using Base = typename FuncPtrWrapper<V>::type;
	using Base<Vs>::operator()...;
};

template<typename... Vi>
Visitor(Vi&&...) -> Visitor<std::remove_pointer_t<std::decay_t<Vi>>...>;

template<typename... Vs>
class UniqueVariant : public std::variant<Vs...>
{
	using Base = std::variant<Vs...>;
public:
	using Base::Base;

    static constexpr std::size_t typeCount() { return sizeof...(Vs); }

    template<typename T> decltype(auto) as() { return std::get<T>(self()); }
    template<typename T> decltype(auto) as() const { return std::get<T>(self()); }

    template<typename T> auto* tryAs() noexcept { return std::get_if<T>(&self()); }
    template<typename T> auto* tryAs() const noexcept { return std::get_if<T>(&self()); }

    template<typename T> bool is() const noexcept { return std::holds_alternative<T>(self()); }

	template<typename... Fs>
	decltype(auto) visit(Fs&&... fs)
    {
        return std::visit(Visitor{std::forward<Fs>(fs)...}, self());
	}
	template<typename... Fs>
	decltype(auto) visit(Fs&&... fs) const
    {
        return std::visit(Visitor{std::forward<Fs>(fs)...}, self());
    }

private:
    Base& self() { return *this; }
    const Base& self() const { return *this; }
};

template<typename Ts, typename... Vs> struct UniqueTypeTrait;

template<typename... Ts, typename V, typename... Vs>
struct UniqueTypeTrait<TList<Ts...>, V, Vs...>
{
	using type = std::conditional_t<
		std::disjunction_v<std::is_same<V, Ts>...>,
		typename UniqueTypeTrait<TList<Ts...   >, Vs...>::type,
		typename UniqueTypeTrait<TList<Ts..., V>, Vs...>::type>;
};

template<typename... Ts>
struct UniqueTypeTrait<TList<Ts...>> { using type = UniqueVariant<Ts...>; };

template<typename... Ts> struct TakeOneTrait;

template<typename T, typename... Ts>
struct TakeOneTrait<T, Ts...>
{
	using type = typename UniqueTypeTrait<TList<T>, Ts...>::type;
};

template<> struct TakeOneTrait<> { using type = UniqueVariant<std::monostate>; };

template<typename... Ts> using Variant = typename TakeOneTrait<Ts...>::type;
}

namespace Variants
{
using Detail::Visitor, Detail::UniqueVariant, Detail::Variant;
}
