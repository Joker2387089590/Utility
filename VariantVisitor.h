#pragma once
#include <variant>
#include "Tuple.h"

namespace Variants::Detail
{
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
	template<typename... VsRef>
	Visitor(VsRef&&... visitors) : Base<Vs>(std::forward<VsRef>(visitors))... {}
	using Base<Vs>::operator()...;
};

template<typename... VsRef>
Visitor(VsRef&&...) -> Visitor<std::remove_pointer_t<std::decay_t<VsRef>>...>;

template<typename... Vs>
class UniqueVariant : public std::variant<Vs...>
{
	using Base = std::variant<Vs...>;
public:
	using Base::Base;

	static std::size_t typeCount() { return sizeof...(Vs); }

	template<typename T> decltype(auto) as() { return std::get<T>(*this); }
	template<typename T> decltype(auto) as() const { return std::get<T>(*this); }

	template<typename T> auto* tryAs() noexcept { return std::get_if<T>(this); }
	template<typename T> auto* tryAs() const noexcept { return std::get_if<T>(this); }

	template<typename T> bool is() const noexcept { return std::holds_alternative(*this); }

	template<typename... Fs>
	decltype(auto) visit(Fs&&... fs)
	{
		return std::visit(Visitor(std::forward<Fs>(fs)...), *this);
	}
	template<typename... Fs>
	decltype(auto) visit(Fs&&... fs) const
	{
		return std::visit(Visitor(std::forward<Fs>(fs)...), *this);
	}

	struct Tag
	{
		Tuple<Vs...> tags{UniqueVariant(Vs{})...};
		template<typename T>
		T& tag() { return tags.template get<T>().template as<T>(); }
	};
	static auto makeTags() { return Tag{}; }
};

template<typename... Ts> struct TList {};

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

template<typename T, typename... Ts>
using Variant = typename UniqueTypeTrait<TList<T>, Ts...>::type;
}

namespace Variants
{
	using Detail::UniqueVariant;
	using Detail::Variant;
}
