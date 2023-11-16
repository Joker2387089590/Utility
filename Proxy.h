#pragma once
#include <Utility/TypeList.h>
#include <Utility/CallableTrait.h>

// 参考 https://github.com/microsoft/proxy
namespace Proxys::Detail
{
/// forward declaration

template<typename... Ds> struct Facade;

template<typename... Ds> class ProxyImpl;

template<typename... Ds> class UniqueProxyImpl;

template<template<typename...> typename P, typename F> struct FacadeTrait;

template<typename F>
using UniqueProxy = typename FacadeTrait<UniqueProxyImpl, F>::Proxys;

template<typename F>
using Proxy = typename FacadeTrait<ProxyImpl, F>::Proxys;

/// dispatch

template<typename F> struct Dispatch;

template<typename R, typename... As>
struct Dispatch<R(As...)>
{
	using Call = Callables::Callable<R(As...)>;
};

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
		return std::invoke(D{}, *static_cast<T*>(object), std::forward<As>(args)...);
	}
};

template<typename T, typename D>
constexpr auto* wrap = Wrap<T, D, typename D::Call::Invoker>::impl;

template<typename D>
struct Unwrap
{
	template<typename R, typename... As>
	struct Impl { using type = R(*)(void*, As...); };

	using type =
		typename D::Call::
		template Expand<Impl>::
		type;
};

template<typename... Ds>
struct Facade
{
	using DList = Types::TList<Ds...>;
	static_assert(DList::template ifAll<IsDispatch>);
	static_assert(!DList::repeated);

	using VTable = std::tuple<typename Unwrap<Ds>::type...>;

	template<typename T>
	inline static constexpr VTable vtable{ wrap<T, Ds>... };
};

auto toFacade(...) -> void;
template<typename... Ds> auto toFacade(const Facade<Ds...>&) -> Facade<Ds...>;

template<template<typename...> typename P, typename F>
struct FacadeTrait : public FacadeTrait<P, decltype(toFacade(std::declval<F>()))> {};

template<template<typename...> typename P, typename... Ds>
struct FacadeTrait<P, Facade<Ds...>> { using Proxys = P<Ds...>; };

/// proxy impl

template<typename... Ds>
class ProxyImpl
{
	using Facades = Facade<Ds...>;
	using VTable = const typename Facades::VTable;
	template<typename... D> friend class UniqueProxyImpl;
public:
	constexpr ProxyImpl() noexcept : object(nullptr), vptr(nullptr) {}

	constexpr ProxyImpl(std::nullptr_t) noexcept : ProxyImpl() {}

	template<typename T>
	constexpr ProxyImpl(T* object) noexcept :
		object(object),
		vptr(std::addressof(Facades::template vtable<T>))
	{}

	template<typename T>
	constexpr ProxyImpl& operator=(T* object) & noexcept
	{
		this->object = object;
		this->vptr = std::addressof(Facades::template vtable<T>);
		return *this;
	}

private:
	constexpr ProxyImpl(void* object, VTable* vptr) : object(object), vptr(vptr) {}

public:
	constexpr ProxyImpl(ProxyImpl&&) noexcept = default;
	constexpr ProxyImpl& operator=(ProxyImpl&&) & noexcept = default;
	constexpr ProxyImpl(const ProxyImpl&) = default;
	constexpr ProxyImpl& operator=(const ProxyImpl&) & = default;
	~ProxyImpl() noexcept = default;

public:
	template<typename D, typename... As>
	decltype(auto) invoke(As&&... args) const
	{
		constexpr auto index = Types::TList<Ds...>::template find<D>();
		static_assert(index != -1);
		return std::get<index>(*vptr)(object, std::forward<As>(args)...);
	}

	constexpr operator bool() const noexcept { return !!object; }

	template<typename T>
	constexpr T* as() const noexcept { return static_cast<T*>(object); }

private:
	void* object;
	VTable* vptr;
};

/// unique proxy impl

struct Destroy : Dispatch<void()>
{
	template<typename T> void operator()(T& obj) { delete &obj; }
};

template<typename... Ds>
class UniqueProxyImpl : public ProxyImpl<Destroy, Ds...>
{
	using Base = ProxyImpl<Destroy, Ds...>;
	template<typename... D> friend class UniqueProxyImpl;
public:
	constexpr UniqueProxyImpl() noexcept : Base() {}
	constexpr UniqueProxyImpl(std::nullptr_t) noexcept : Base() {}

public: // move only
	UniqueProxyImpl(const UniqueProxyImpl&) = delete;
	UniqueProxyImpl& operator=(const UniqueProxyImpl&) & = delete;

	UniqueProxyImpl(UniqueProxyImpl&& other) noexcept : UniqueProxyImpl()
	{
		*this = std::move(other);
	}

	UniqueProxyImpl& operator=(UniqueProxyImpl&& other) & noexcept
	{
		std::swap(this->object, other.object);
		std::swap(this->vptr, other.vptr);
		return *this;
	}

	~UniqueProxyImpl() { if(this->object) this->template invoke<Destroy>(); }

public: // non-trivial contructors
	// add explicit
	template<typename T>
	explicit constexpr UniqueProxyImpl(T* object) : Base(object) {}

	// the deleter cannot be created
	template<typename... D>
	UniqueProxyImpl(ProxyImpl<D...> proxy) = delete;

public: // to normal proxy
	operator ProxyImpl<Ds...>() const { return static_cast<const Base&>(*this); }

public: // Base function
	using Base::invoke;
	using Base::operator bool;
	using Base::as;
};
} // namespace Proxys::Detail

namespace Proxys
{
using Detail::Dispatch;
using Detail::Facade;
using Detail::Proxy;
using Detail::UniqueProxy;
}
