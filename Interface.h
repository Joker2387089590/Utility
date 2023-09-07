#pragma once
#include <memory>
#include <type_traits>
#include <Utility/CallableTrait.h>

namespace Interfaces
{
template<typename F>
struct Dispatch : public Dispatch<typename Callables::FunctorInvoker<F>> {};

template<typename R, typename... As>
struct Dispatch<R(As...)> : Callables::Callable<R(As...)>
{

};

template<typename T> auto isDispatch(const T&) -> std::false_type;
template<typename F> auto isDispatch(const Dispatch<F>&) -> std::true_type;
template<typename T> using IsDispatch = decltype(isDispatch(std::declval<T>()));

template<typename T, typename D>
auto build()
{
	return [](void* object, auto&&... args) {
		auto self = static_cast<T*>(object);
		return D{}(self, std::forward<decltype(args)>(args)...);
	};
}

template<typename... Ds>
struct Facade
{
	static_assert(std::conjunction_v<IsDispatch<Ds>...>);
};

template<typename F> struct Proxy;

template<typename... Ds>
struct Proxy<Facade<Ds...>>
{

	template<typename T>
	Proxy(T& object) : object(&object)
	{

	}

	template<typename D, typename... As>
	decltype(auto) invoke(As&&... args);

	void* object;
};
} // namespace Interfaces
