#pragma once
#include <type_traits>
#include <Utility/CallableTrait.h>
#include <Utility/Macros.h>

// 参考 https://github.com/microsoft/proxy
namespace Proxys::Detail
{
template<typename F>
struct Dispatch : public Dispatch<typename Callables::FunctorInvoker<F>> {};

template<typename R, typename... As>
struct Dispatch<R(As...)> : Callables::Callable<R(As...)> {};

auto isDispatch(...) -> std::false_type;
template<typename F> auto isDispatch(const Dispatch<F>&) -> std::true_type;
template<typename T> using IsDispatch = decltype(isDispatch(std::declval<T>()));

template<typename T, typename D, typename Invoker> struct Wrap;

template<typename T, typename D, typename R, typename... As>
struct Wrap<T, D, R(As...)>
{
	static constexpr auto (*impl)(void*, As...) -> R = [](void* object, As... args) -> R {
		auto self = static_cast<T*>(object);
		return std::invoke(D{}, self, std::forward<As>(args)...);
	};
};

template<typename T, typename D>
constexpr auto wrap() { return Wrap<T, D, typename D::Invoker>::impl; }

template<typename D>
struct Unwrap
{
	template<typename R, typename... As> struct Impl { using type = R(*)(void*, As...); };
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
	inline static constexpr std::tuple vtable{ wrap<T, Ds>()... };

	using VTable = std::tuple<typename Unwrap<Ds>::type...>;
};

auto toFacade(...) -> void;
template<typename... Ds> auto toFacade(const Facade<Ds...>&) -> Facade<Ds...>;

template<typename F> class Proxy : public Proxy<decltype(toFacade(std::declval<F>()))> {};

template<typename... Ds>
class Proxy<Facade<Ds...>>
{
	using F = Facade<Ds...>;
public:
	template<typename T>
	Proxy(T* object) : object(object), vptr(&F::template vtable<T>) {}

	DefaultClass(Proxy);

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
};
} // namespace Interfaces::Detail

namespace Proxys
{
using Detail::Dispatch;
using Detail::Facade;
using Detail::Proxy;
} // namespace Interfaces

#include <Utility/MacrosUndef.h>
