#pragma once
#include <type_traits>
#include <Utility/CallableTrait.h>
#include <Utility/Macros.h>

// 参考 https://github.com/microsoft/proxy
namespace Proxys::Detail
{
/// forward declaration
template<typename... Ds> class ProxyImpl;

template<typename... Ds> class UniqueProxyImpl;

template<template<typename...> typename P, typename F> struct FacadeTrait;

template<typename F>
using UniqueProxy = typename FacadeTrait<UniqueProxyImpl, F>::Proxys;

template<typename F>
using Proxy = typename FacadeTrait<ProxyImpl, F>::Proxys;

/// dispatch

template<typename F>
struct Dispatch : public Dispatch<typename Callables::FunctorInvoker<F>> {};

template<typename R, typename... As>
struct Dispatch<R(As...)> : Callables::Callable<R(As...)> {};

auto isDispatch(...) -> std::false_type;
template<typename F> auto isDispatch(const Dispatch<F>&) -> std::true_type;
template<typename T> using IsDispatch = decltype(isDispatch(std::declval<T>()));

/// facade

template<typename T, typename D, typename Invoker> struct Wrap;

template<typename T, typename D, typename R, typename... As>
struct Wrap<T, D, R(As...)>
{
	static constexpr R impl(void* object, As... args)
	{
		return std::invoke(D{}, static_cast<T*>(object), std::forward<As>(args)...);
	}
};

template<typename T, typename D>
constexpr auto* wrap = Wrap<T, D, typename D::Invoker>::impl;

template<typename D>
struct Unwrap
{
	template<typename R, typename... As>
	struct Impl { using type = R(*)(void*, As...); };

	using type =
		typename D::Arguments::
		template Prepend<typename D::Return>::
		template Apply<Impl>::
		type;
};

template<typename... Ds>
struct Facade
{
	static_assert(std::conjunction_v<IsDispatch<Ds>...>);

	template<typename T>
	inline static constexpr std::tuple vtable{ wrap<T, Ds>... };

	using VTable = std::tuple<typename Unwrap<Ds>::type...>;
};

auto toFacade(...) -> void;
template<typename... Ds> auto toFacade(const Facade<Ds...>&) -> Facade<Ds...>;

template<template<typename...> typename P, typename F>
struct FacadeTrait : public FacadeTrait<P, decltype(toFacade(std::declval<F>()))> {};

template<template<typename...> typename P, typename... Ds>
struct FacadeTrait<P, Facade<Ds...>> { using Proxys = P<Ds...>; };

/// unique proxy impl

template<typename... Ds>
class UniqueProxyImpl
{
	using F = Facade<Ds...>;
	friend class ProxyImpl<Ds...>;

	template<typename T>
	static constexpr void(*defaultDeleter)(void*) =
		[](void* obj) -> void { delete static_cast<T*>(obj); };

public:
	UniqueProxyImpl() noexcept : object(nullptr), deleter(nullptr), vptr(nullptr) {}

	UniqueProxyImpl(UniqueProxyImpl&& other) noexcept : UniqueProxyImpl()
	{
		(*this) = std::move(other);
	}

	UniqueProxyImpl& operator=(UniqueProxyImpl&& other) & noexcept
	{
		std::swap(object, other.object);
		std::swap(deleter, other.deleter);
		std::swap(vptr, other.vptr);
	}

	UniqueProxyImpl(const UniqueProxyImpl&) = delete;
	UniqueProxyImpl& operator=(const UniqueProxyImpl&) & = delete;

	template<typename T>
	explicit UniqueProxyImpl(T* object) :
		object(object),
		vptr(std::addressof(F::template vtable<T>)),
		deleter(defaultDeleter<T>)
	{}

	template<typename T>
	UniqueProxyImpl(T* object, void (*deleter)(void*)) :
		object(object),
		vptr(std::addressof(F::template vtable<T>)),
		deleter(deleter)
	{}

	~UniqueProxyImpl() { deleter(object); }

public:
	template<typename D, typename... As>
	decltype(auto) invoke(As&&... args) const
	{
		constexpr auto index = Types::TList<Ds...>::template find<D>();
		static_assert(index != -1);
		return std::get<index>(*vptr)(object, std::forward<As>(args)...);
	}

	template<typename T> T* as() const { return static_cast<T*>(object); }

private:
	void* object;
	const typename F::VTable* vptr;
	void (*deleter)(void*);
};

/// proxy impl

template<typename... Ds>
class ProxyImpl
{
	using F = Facade<Ds...>;
public:
	ProxyImpl() : object(nullptr), vptr(nullptr) {}

	ProxyImpl(ProxyImpl&&) noexcept = default;
	ProxyImpl& operator=(ProxyImpl&&) & noexcept = default;
	ProxyImpl(const ProxyImpl&) = default;
	ProxyImpl& operator=(const ProxyImpl&) & = default;
	~ProxyImpl() noexcept = default;

	template<typename T>
	ProxyImpl(T* object) :
		object(object),
		vptr(std::addressof(F::template vtable<T>))
	{}

	ProxyImpl(const UniqueProxyImpl<Ds...>& u) : object(u.object), vptr(u.vptr) {}

public:
	template<typename D, typename... As>
	decltype(auto) invoke(As&&... args) const
	{
		constexpr auto index = Types::TList<Ds...>::template find<D>();
		static_assert(index != -1);
		return std::get<index>(*vptr)(object, std::forward<As>(args)...);
	}

	template<typename T> T* as() const noexcept { return static_cast<T*>(object); }

	operator bool() const noexcept { return !!object; }

private:
	void* object;
	const typename F::VTable* vptr;
};
} // namespace Interfaces::Detail

namespace Proxys
{
using Detail::Dispatch;
using Detail::Facade;
using Detail::Proxy;
using Detail::UniqueProxy;
} // namespace

#include <Utility/MacrosUndef.h>
