#pragma once
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
template<typename T> struct SelectFunctor<T, true> { using type = typename IsFunctor<T>::type; };

template<typename F>
using FunctorType = typename SelectFunctor<F, IsFunctor<F>::value>::type;

template<typename F>
struct Callable : Callable<FunctorType<std::decay_t<F>>> {};

template<typename R, typename... Args>
struct Callable<R(Args...)> // C function
{
	using Return = R;
	using Class = void;
	using Arguments = TList<Args...>;
	using Invoker = R(Args...);
};

// C function pointer
template<typename R, typename... Args>
struct Callable<R(*)(Args...)> : Callable<R(Args...)> {};

// C function ref
template<typename R, typename... Args>
struct Callable<R(&)(Args...)> : Callable<R(Args...)> {};

// member function
template<typename R, typename T, typename... Args>
struct Callable<R(T::*)(Args...)> : Callable<R(Args...)>
{
	using Class = T;
};

// const member function
template<typename R, typename T, typename... Args>
struct Callable<R(T::*)(Args...) const> : Callable<R(Args...)>
{
	using Class = const T;
};

// left-ref member function
template<typename R, typename T, typename... Args>
struct Callable<R(T::*)(Args...) &> : Callable<R(Args...)>
{
	using Class = T&;
};

// right-ref member function
template<typename R, typename T, typename... Args>
struct Callable<R(T::*)(Args...) &&> : Callable<R(Args...)>
{
	using Class = T&&;
};

// const-left-ref member function
template<typename R, typename T, typename... Args>
struct Callable<R(T::*)(Args...) const&> : Callable<R(Args...)>
{
	using Class = const T&;
};

// const-right-ref member function
template<typename R, typename T, typename... Args>
struct Callable<R(T::*)(Args...) const&&> : Callable<R(Args...)>
{
	using Class = const T&&;
};

// std::function
template<typename F>
struct Callable<std::function<F>> : Callable<F> {};

template<typename F> using FunctorReturn    = typename Callable<F>::Return;
template<typename F> using FunctorClass     = typename Callable<F>::Class;
template<typename F> using FunctorArguments = typename Callable<F>::Arguments;
template<typename F> using FunctorInvoker   = typename Callable<F>::Invoker;

template<auto f> using FunctorInfo = Callable<decltype(f)>;
template<auto f> using ReturnOf    = typename FunctorInfo<f>::Return;
template<auto f> using ClassOf     = typename FunctorInfo<f>::Class;
template<auto f> using ArgumentsOf = typename FunctorInfo<f>::Arguments;
template<auto f> using InvokerOf   = typename FunctorInfo<f>::Invoker;

template<typename Fr>
struct InvokeTypedFunctor : Callable<Fr>
{
	using Return = FunctorReturn<Fr>;
	using Arguments = FunctorArguments<Fr>;

	template<typename R, typename... Args, typename F>
	static constexpr auto wrap(F&& f, TList<Args...>)
	{
		return [f = std::forward<F>(f)](Args... ts) constexpr -> R {
			return f(std::forward<Args>(ts)...);
		};
	}

	template<typename F>
	friend constexpr auto operator|(F&& f, InvokeTypedFunctor)
	{
		return wrap<Return>(std::forward<F>(f), Arguments{});
	}
};

template<typename R>
struct ReturnTypedFunctor
{
	template<typename... Args, typename F>
	static constexpr auto wrap(F&& f, TList<Args...>)
	{
		return [f = std::forward<F>(f)](Args... ts) constexpr -> R {
			return R(f(std::forward<Args>(ts)...));
		};
	}

	template<typename F>
	friend constexpr auto operator|(F&& f, ReturnTypedFunctor)
	{
		if constexpr (IsFunctor<F>::value)
		{
			using Arguments = FunctorArguments<F>;
			return wrap(std::forward<F>(f), Arguments{});
		}
		else
		{
			// f may be a functor with overloaded calling operators
			return [f = std::forward<F>(f)](auto&&... args) constexpr -> R {
				return R(f(std::forward<decltype(args)>(args)...));
			};
		}
	}
};

template<typename... Args>
struct ArgTypedFunctor
{
	template<typename F>
	friend constexpr auto operator|(F&& f, ArgTypedFunctor)
	{
		return [f = std::forward<F>(f)](Args... args) constexpr {
			return f(std::forward<Args>(args)...);
		};
	}
};

namespace cpo
{
/// [...](auto...) -> auto {...} | invokeAs<int(int)> => [...](int) -> int {...}
template<typename Fr>
inline constexpr auto invokeAs = InvokeTypedFunctor<Fr>{};

/// [...](auto...) -> auto {...} | returnAs<int> => [...](auto...) -> int {...}
template<typename R>
inline constexpr auto returnAs = ReturnTypedFunctor<R>{};

/// [...](auto...) -> auto {...} | argAs<int, int> => [...](int, int) -> auto {...}
template<typename... Args>
inline constexpr auto argAs = ArgTypedFunctor<Args...>{};
}

template<typename Fr, typename F>
[[deprecated("use invokeAs CPO instead")]]
constexpr auto typedLambda(F&& f)
{
	return std::forward<F>(f) | cpo::invokeAs<Fr>;
}

template<typename F> struct FuncPtrWrapper { using type = F; };

template<typename R, typename... Args>
struct FuncPtrWrapper<R(Args...)>
{
	using FuncPtr = R(*)(Args...);
	struct type
	{
		constexpr type(FuncPtr f) noexcept : f(f) {}
		constexpr R operator()(Args... args) const { return f(std::forward<Args>(args)...); }
		FuncPtr f;
	};
};

template<typename... Vs>
struct Visitor : FuncPtrWrapper<Vs>::type...
{
	template<typename V>
	using Base = typename FuncPtrWrapper<V>::type;

	template<typename... Vi>
	constexpr Visitor(Vi&&... fs) : Base<Vs>(std::forward<Vi>(fs))... {}

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

using Detail::FunctorInfo;
using Detail::ReturnOf;
using Detail::ClassOf;
using Detail::ArgumentsOf;
using Detail::InvokerOf;

namespace cpo = Detail::cpo;
using cpo::invokeAs;
using cpo::returnAs;
using cpo::argAs;
using Detail::typedLambda;

using Detail::Visitor;
}
