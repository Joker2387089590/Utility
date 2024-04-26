#pragma once
#include <functional>
#include <Utility/TypeList.h>
#include <Utility/Macros.h>

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
template<typename T>
struct SelectFunctor<T, false> { using type = T; };
template<typename T>
struct SelectFunctor<T, true> { using type = typename IsFunctor<T>::type; };

template<typename F>
using FunctorType = typename SelectFunctor<F, IsFunctor<F>::value>::type;

template<typename T, typename R, typename... Args>
struct CallableImpl
{
	using Class = T;
	using Return = R;
	using Arguments = TList<Args...>;
	using Invoker = R(Args...);

	template<template<typename Rx, typename... Ax> typename E>
	using Expand = E<R, Args...>;

	template<template<typename Tx, typename Rx, typename... Ax> typename E>
	using ExpandClass = E<T, R, Args...>;
};

template<typename F> struct CallableTrait;

// functor(lambda) and fallback
template<typename F>
struct CallableTrait : CallableTrait<FunctorType<std::decay_t<F>>> {};

// C function pointer
template<typename R, typename... Args>
struct CallableTrait<R(*)(Args...)>
{
	using type = CallableImpl<void, R, Args...>;
};

// member function
template<typename R, typename T, typename... Args>
struct CallableTrait<R(T::*)(Args...)>
{
	using type = CallableImpl<T, R, Args...>;
};

// const member function
template<typename R, typename T, typename... Args>
struct CallableTrait<R(T::*)(Args...) const>
{
	using type = CallableImpl<const T, R, Args...>;
};

// left-ref member function
template<typename R, typename T, typename... Args>
struct CallableTrait<R(T::*)(Args...) &>
{
	using type = CallableImpl<T&, R, Args...>;
};

// right-ref member function
template<typename R, typename T, typename... Args>
struct CallableTrait<R(T::*)(Args...) &&>
{
	using type = CallableImpl<T&&, R, Args...>;
};

// const-left-ref member function
template<typename R, typename T, typename... Args>
struct CallableTrait<R(T::*)(Args...) const&>
{
	using type = CallableImpl<const T&, R, Args...>;
};

// const-right-ref member function
template<typename R, typename T, typename... Args>
struct CallableTrait<R(T::*)(Args...) const&&>
{
	using type = CallableImpl<const T&&, R, Args...>;
};

// std::function
template<typename F>
struct CallableTrait<std::function<F>> : CallableTrait<F> {};

template<typename F>
using Callable = typename CallableTrait<F>::type;

template<auto f>
using CallableOf = Callable<decltype(f)>;

template<typename F> using FunctorReturn    = typename Callable<F>::Return;
template<typename F> using FunctorClass     = typename Callable<F>::Class;
template<typename F> using FunctorArguments = typename Callable<F>::Arguments;
template<typename F> using FunctorInvoker   = typename Callable<F>::Invoker;

template<typename F, template<typename... Ts> typename E>
using Expand = typename Callable<F>::template Expand<E>;

template<typename F, template<typename... Ts> typename E>
using ExpandClass = typename Callable<F>::template ExpandClass<E>;

template<auto f> using ReturnOf    = typename CallableOf<f>::Return;
template<auto f> using ClassOf     = typename CallableOf<f>::Class;
template<auto f> using ArgumentsOf = typename CallableOf<f>::Arguments;
template<auto f> using InvokerOf   = typename CallableOf<f>::Invoker;

template<auto f, template<typename... Ts> typename E>
using ExpandOf = typename CallableOf<f>::template Expand<E>;

template<auto f, template<typename... Ts> typename E>
using ExpandClassOf = typename CallableOf<f>::template ExpandClass<E>;

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
/// [...](auto...) -> auto {...} | invokeAs<int(int)>
///		=> [...](int) -> int {...}
template<typename Fr>
inline constexpr auto invokeAs = InvokeTypedFunctor<Fr>{};

/// [...](auto...) -> auto {...} | returnAs<int>
///		=> [...](auto...) -> int {...}
template<typename R>
inline constexpr auto returnAs = ReturnTypedFunctor<R>{};

/// [...](auto...) -> auto {...} | argAs<int, int>
///		=> [...](int, int) -> auto {...}
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

template<typename F, typename... Args>
constexpr auto curry(F&& f, Args&&... args1)
{
	// 参考 https://stackoverflow.com/a/49902823
	return [f = fwd(f), args1 = std::make_tuple(std::forward<Args>(args1)...)](auto&&... args2) mutable {
		return std::apply([&](auto&&... args1) {
			return std::invoke(fwd(f), std::forward<Args>(args1)..., fwd(args2)...);
		}, std::move(args1));
	};
}

template<typename T, typename R, typename... As>
struct BindImpl
{
	template<auto f, typename Tr>
	struct ZeroCostFunc
	{
		constexpr R operator()(As... args) const
		{
			return (obj->*f)(std::forward<As>(args)...);
		}
		Tr* const obj;
	};

	template<typename Tr>
	struct Func
	{
		constexpr R operator()(As... args) const
		{
			return (obj->*f)(std::forward<As>(args)...);
		}
		Tr* const obj;
		R(T::* const f)(As...);
	};
};

template<auto f, typename T>
constexpr auto bind(T* obj) noexcept
{
	using Impl = typename CallableOf<f>::template ExpandClass<BindImpl>;
	using Func = typename Impl::template ZeroCostFunc<f, T>;
	return Func{obj};
}

template<typename T, typename F>
constexpr auto bind(T* obj, F f) noexcept
{
	using Impl = typename Callable<F>::template ExpandClass<BindImpl>;
	using Func = typename Impl::template Func<T>;
	return Func{obj, f};
}

template<typename T, typename R, typename... Args>
struct UnbindImpl
{
	template<auto f>
	static constexpr R impl(T* obj, Args... args)
		noexcept(noexcept((obj->*f)(std::forward<Args>(args)...)))
	{
		return (obj->*f)(std::forward<Args>(args)...);
	}
};

template<auto memberFunc>
constexpr auto unbind = ExpandClassOf<memberFunc, UnbindImpl>::template impl<memberFunc>;
}

namespace Callables
{
using Detail::Callable;
using Detail::CallableOf;

using Detail::FunctorReturn;
using Detail::FunctorClass;
using Detail::FunctorArguments;
using Detail::FunctorInvoker;
using Detail::Expand;
using Detail::ExpandClass;

using Detail::ReturnOf;
using Detail::ClassOf;
using Detail::ArgumentsOf;
using Detail::InvokerOf;
using Detail::ExpandOf;
using Detail::ExpandClassOf;

namespace cpo = Detail::cpo;
using cpo::invokeAs;
using cpo::returnAs;
using cpo::argAs;
using Detail::typedLambda;

using Detail::Visitor;
using Detail::bind;
using Detail::unbind;
}

#include <Utility/MacrosUndef.h>
