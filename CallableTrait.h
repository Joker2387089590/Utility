#pragma once
#include <type_traits>
#include <Utility/Tuple.h>

namespace Callables::Detail
{
template<typename T, typename = void>
struct IsFunctor : std::false_type {};

template<typename T>
struct IsFunctor<T, std::void_t<decltype(&T::operator())>> : std::true_type
{
	using type = decltype(&T::operator());
};

template<typename T, bool b> struct SelectFunctor;
template<typename T> struct SelectFunctor<T, false> {};
template<typename T> struct SelectFunctor<T, true>  { using type = typename IsFunctor<T>::type; };

template<typename F>
using FunctorType = typename SelectFunctor<F, IsFunctor<F>::value>::type;

template<typename F>
struct Callable : Callable<FunctorType<std::decay_t<F>>> {};

template<typename R, typename T, typename... Args>
struct Callable<R(T::*)(Args...)>
{
	using Return = R;
	using Class = T;
	using Arguments = Tuples::Tuple<Args...>;
	using Invoker = R(Args...);
};

template<typename R, typename T, typename... Args>
struct Callable<R(T::*)(Args...) const>
{
	using Return = R;
	using Class = const T;
	using Arguments = Tuples::Tuple<Args...>;
	using Invoker = R(Args...);
};

template<typename R, typename T, typename... Args>
struct Callable<R(T::*)(Args...) &>
{
	using Return = R;
	using Class = T&;
	using Arguments = Tuples::Tuple<Args...>;
	using Invoker = R(Args...);
};

template<typename R, typename T, typename... Args>
struct Callable<R(T::*)(Args...) &&>
{
	using Return = R;
	using Class = T&&;
	using Arguments = Tuples::Tuple<Args...>;
	using Invoker = R(Args...);
};

template<typename R, typename T, typename... Args>
struct Callable<R(T::*)(Args...) const&>
{
	using Return = R;
	using Class = const T&;
	using Arguments = Tuples::Tuple<Args...>;
	using Invoker = R(Args...);
};

template<typename R, typename... Args>
struct Callable<R(*)(Args...)>
{
	using Return = R;
	using Class = void;
	using Arguments = Tuples::Tuple<Args...>;
	using Invoker = R(Args...);
};

template<typename R, typename... Args>
struct Callable<R(Args...)>
{
	using Return = R;
	using Class = void;
	using Arguments = Tuples::Tuple<Args...>;
	using Invoker = R(Args...);
};

template<typename F> using FunctorReturn    = typename Callable<F>::Return;
template<typename F> using FunctorClass     = typename Callable<F>::Class;
template<typename F> using FunctorArguments = typename Callable<F>::Arguments;
template<typename F> using FunctorInvoker   = typename Callable<F>::Invoker;

template<typename F, typename R, typename... Args>
constexpr auto typedLambdaHelper(F&& f, R*, Tuples::Tuple<Args...>*)
{
	return [f = std::forward<F>(f)](Args... ts) -> R {
		return f(std::forward<Args>(ts)...);
	};
}

template<typename Fr, typename F>
constexpr auto typedLambda(F&& f)
{
	using Call = Callable<Fr>;
	using Rp = std::add_pointer_t<typename Call::Return>;
	using Ap = std::add_pointer_t<typename Call::Arguments>;
	return typedLambdaHelper(std::forward<F>(f), Rp{}, Ap{});
}

}

namespace Callables
{
using Detail::Callable;
using Detail::FunctorReturn;
using Detail::FunctorClass;
using Detail::FunctorArguments;
using Detail::FunctorInvoker;
using Detail::typedLambda;
}
