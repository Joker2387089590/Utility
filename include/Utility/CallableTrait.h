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

template<typename F>
constexpr bool isFunction = std::is_function_v<std::remove_pointer_t<std::decay_t<F>>>;

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
			return static_cast<R>(f(std::forward<Args>(ts)...));
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
			return static_cast<R>(f(std::forward<Args>(ts)...));
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
				return static_cast<R>(f(std::forward<decltype(args)>(args)...));
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
template<typename Fr>
inline constexpr auto invokeAs = InvokeTypedFunctor<Fr>{};

template<typename R>
inline constexpr auto returnAs = ReturnTypedFunctor<R>{};

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

template<typename T> struct RefTrait { using type = T&; };
template<typename T> struct RefTrait<T&&> { using type = T&&; };
template<typename T> using ForceRef = typename RefTrait<T>::type;

template<typename T, typename R, typename... As>
struct BindImpl
{
	template<auto f, typename Tr>
	struct ZeroCostFunc
	{
		constexpr R operator()(As... args) const
		{
			return std::invoke(f, static_cast<ForceRef<T>>(*obj), std::forward<As>(args)...);
		}
		Tr* const obj;
	};

	template<typename F, typename Tr>
	struct Func
	{
		constexpr R operator()(As... args) const
		{
			return std::invoke(f, static_cast<ForceRef<T>>(*obj), std::forward<As>(args)...);
		}
		Tr* const obj;
		F f;
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
	using Func = typename Impl::template Func<F, T>;
	return Func{obj, f};
}

template<typename T, typename R, typename... Args>
struct UnbindImpl
{
	template<auto f>
	struct Impl
	{
		constexpr R operator()(std::remove_reference_t<T>* obj, Args... args) const
		{
			return std::invoke(f, static_cast<ForceRef<T>>(*obj), std::forward<Args>(args)...);
		}

		constexpr R operator()(ForceRef<T> obj, Args... args) const
		{
			return std::invoke(f, static_cast<ForceRef<T>>(obj), std::forward<Args>(args)...);
		}
	};
};

template<auto memberFunc>
constexpr auto unbind = typename ExpandClassOf<memberFunc, UnbindImpl>::template Impl<memberFunc>{};

template<typename R, typename... Args>
class FunctionRefImpl
{
public:
	template<typename Rx, typename... Ax>
	FunctionRefImpl(Rx(*f)(Ax...)) noexcept :
		target(reinterpret_cast<void(*)()>(f)),
		func([](Target target, Args&&... args) -> R {
			return ((Rx(*)(Ax...))(target.cfunc))(std::forward<Args>(args)...);
		})
	{}

	template<typename F, std::enable_if_t<!isFunction<F>, int> = 0>
	constexpr FunctionRefImpl(F&& f) noexcept :
		target(static_cast<const void*>(std::addressof(f))),
		func([](Target target, Args&&... args) -> R {
			auto of = static_cast<std::add_pointer_t<F>>(const_cast<void*>(target.obj));
			return std::invoke(*of, std::forward<Args>(args)...);
		})
	{}

	constexpr FunctionRefImpl(const FunctionRefImpl&) noexcept = default;
	constexpr FunctionRefImpl& operator=(const FunctionRefImpl&) & noexcept = default;
	constexpr FunctionRefImpl(FunctionRefImpl&&) noexcept = default;
	constexpr FunctionRefImpl& operator=(FunctionRefImpl&&) & noexcept = default;

	constexpr R operator()(Args... args) const
	{
		return std::invoke(func, target, std::forward<Args>(args)...);
	}

private:
	union Target
	{
		constexpr explicit Target(const void* obj) noexcept : obj(obj) {}
		constexpr explicit Target(void(*cfunc)()) noexcept : cfunc(cfunc) {}

		const void* obj;
		void(*cfunc)();
	};
	Target target;
	R(*func)(Target, Args&&...);
};

template<typename F>
using FunctionRef = Expand<F, FunctionRefImpl>;

template<typename T, bool useInitialList = false>
inline constexpr auto constructor = [](auto&&... args) {
	if constexpr (useInitialList)
		return T(std::forward<decltype(args)>(args)...);
	else
		return T{std::forward<decltype(args)>(args)...};
};

template<typename T, bool useInitialList = false>
inline constexpr auto creator = [](auto&&... args) {
	if constexpr (useInitialList)
		return new T(std::forward<decltype(args)>(args)...);
	else
		return new T{std::forward<decltype(args)>(args)...};
};
} // namespace Callables::Detail

namespace Callables
{
using Detail::Callable;
using Detail::CallableOf;

/// using F = double (T::*)(int, char);
/// using V = FunctorReturn<F>; // V == double
using Detail::FunctorReturn;

/// using F = double (T::*)(int, char);
/// using V = FunctorClass<F>; // V == T
using Detail::FunctorClass;

/// using F = double (T::*)(int, char);
/// using V = FunctorArguments<F>; // V == Types::TList<int, char>
using Detail::FunctorArguments;

/// using F = double (T::*)(int, char);
/// using V = FunctorInvoker<F>; // V == double(int, char)
using Detail::FunctorInvoker;

/// template<typename R, typename... As> struct MyTemplate {};
/// using F = double (T::*)(int, char);
/// using V = Expand<F, MyTemplate>; // V == MyTemplate<double, int, char>
using Detail::Expand;

/// template<typename R, typename... As> struct MyTemplate {};
/// using F = double (T::*)(int, char) const&;
/// using V = ExpandClass<F, MyTemplate>; // V == MyTemplate<const T&, double, int, char>
using Detail::ExpandClass;

/// struct A { int foo(); };
/// A& foo2();
/// using R = ReturnOf<&A::foo>; // R == int
/// using C2 = ClassOf<foo2>; // C2 == A&
using Detail::ReturnOf;

/// struct A { void foo() const&&; };
/// A& foo2();
/// using C = ClassOf<&A::foo>; // C == const A&&
/// using C2 = ClassOf<foo2>; // C2 == void
using Detail::ClassOf;

/// struct A { void foo(int, char, double); };
/// using As = Arguments<&A::foo>; // As == Types::TList<int, char, double>
using Detail::ArgumentsOf;

/// struct A { double foo(int, char); };
/// using I = InvokerOf<&A::foo>; // I == double(int, char)
using Detail::InvokerOf;

/// template<typename R, typename... As> struct MyTemplate {};
/// struct A { double foo(int, char); };
/// using My = ExpandOf<&A::foo, MyTemplate>; // My == MyTemplate<double, int, char>
using Detail::ExpandOf;

/// template<typename T, typename R, typename... As> struct MyTemplate {};
/// struct A { double foo(int, char) const&&; };
/// using My = ExpandClassOf<&A::foo, MyTemplate>;
/// // My == MyTemplate<const A&&, double, int, char>
using Detail::ExpandClassOf;

namespace cpo = Detail::cpo;

/// auto generic = [...](auto...) -> auto {...};
/// auto typed = generic | invokeAs<int(int)>;
///	int r = typed(10); // int(generic(10));
using cpo::invokeAs;

/// [...](auto...) -> auto {...} | returnAs<int>
///		=> [...](auto...) -> int {...}
using cpo::returnAs;

/// [...](auto...) -> auto {...} | argAs<int, int>
///		=> [...](int, int) -> auto {...}
using cpo::argAs;

using Detail::typedLambda;

/// https://www.modernescpp.com/index.php/visiting-a-std-variant-with-the-overload-pattern/
using Detail::Visitor;

/// https://en.wikipedia.org/wiki/Currying
using Detail::curry;

/// struct A { void foo(int, int); };
/// A a;
///
/// auto foo = bind<&A::foo>(&a);
/// foo(0, 1); // == a.foo(0, 1);
/// static_assert(sizeof(foo) == sizeof(void*));
///
/// auto foo2 = bind(&a, &A::foo);
/// foo2(0, 1); // == a.foo(0, 1);
/// static_assert(sizeof(foo2) == 2 * sizeof(void*));
using Detail::bind;

/// struct A {
///		void fn(int);
///		void constFn(int) const;
/// };
///
///	A a;
///	constexpr auto fn = unbind<&A::fn>;
/// fn(&a, 1); // == a.fn(1);
/// fn(a, 1);  // == a.fn(1);
///
/// const A ca;
/// constexpr auto cfn = unbind<&A::constFn>;
/// cfn(&ca, 1); // == ca.constFn(1);
using Detail::unbind;

/// double foo(FunctionRef<double(char))> f) { return f('0'); }
/// double x = foo([](char c) -> double { return c + 1; }); // x == '0' + 1
using Detail::FunctionRef;

/// struct T1 { T1(int); T1(std::string); };
/// struct T2 { T2(int); T2(std::string); };
/// std::any fromInt(int i, FunctionRef<std::any(int)> f) { return f(i); }
/// std::any fromStr(std::string s, FunctionRef<std::any(std::string)> f) { return f(s); }
/// auto x = fromInt(0, constructor<T1>); // std::any x = T1(0);
/// auto y = fromStr("str", constructor<T2>); // std::any y = T2("str");
using Detail::constructor;
using Detail::creator;
}

#include <Utility/MacrosUndef.h>
