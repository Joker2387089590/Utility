#pragma once
#include <type_traits>
#include <functional>
#include <Utility/TypeList.h>

namespace Callables::Detail
{
using Types::TList;

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

template<typename F>
struct Callable<std::function<F>> : Callable<F> {};

template<typename R, typename T, typename... Args>
struct Callable<R(T::*)(Args...)>
{
	using Return = R;
	using Class = T;
	using Arguments = TList<Args...>;
	using Invoker = R(Args...);
};

template<typename R, typename T, typename... Args>
struct Callable<R(T::*)(Args...) const>
{
	using Return = R;
	using Class = const T;
	using Arguments = TList<Args...>;
	using Invoker = R(Args...);
};

template<typename R, typename T, typename... Args>
struct Callable<R(T::*)(Args...) &>
{
	using Return = R;
	using Class = T&;
	using Arguments = TList<Args...>;
	using Invoker = R(Args...);
};

template<typename R, typename T, typename... Args>
struct Callable<R(T::*)(Args...) &&>
{
	using Return = R;
	using Class = T&&;
	using Arguments = TList<Args...>;
	using Invoker = R(Args...);
};

template<typename R, typename T, typename... Args>
struct Callable<R(T::*)(Args...) const&>
{
	using Return = R;
	using Class = const T&;
	using Arguments = TList<Args...>;
	using Invoker = R(Args...);
};

template<typename R, typename... Args>
struct Callable<R(*)(Args...)> : Callable<R(Args...)> {};

template<typename R, typename... Args>
struct Callable<R(Args...)>
{
	using Return = R;
	using Class = void;
	using Arguments = TList<Args...>;
	using Invoker = R(Args...);
};

template<typename F> using FunctorReturn    = typename Callable<F>::Return;
template<typename F> using FunctorClass     = typename Callable<F>::Class;
template<typename F> using FunctorArguments = typename Callable<F>::Arguments;
template<typename F> using FunctorInvoker   = typename Callable<F>::Invoker;

template<typename R, typename As> struct TypedLambdaHelper;

template<typename R, typename... Args>
struct TypedLambdaHelper<R, TList<Args...>>
{
	template<typename F>
	static auto wrap(F&& f)
	{
		return [f = std::forward<F>(f)](Args... ts) -> R {
			return f(std::forward<Args>(ts)...);
		};
	}
};

template<typename Fr, typename F>
constexpr auto typedLambda(F&& f)
{
	using Call = Callable<Fr>;
	using Helper = TypedLambdaHelper<typename Call::Return, typename Call::Arguments>;
	return Helper::wrap(std::forward<F>(f));
}

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
}

namespace Callables
{
using Detail::Callable;
using Detail::FunctorReturn;
using Detail::FunctorClass;
using Detail::FunctorArguments;
using Detail::FunctorInvoker;
using Detail::typedLambda;
using Detail::Visitor;
}
