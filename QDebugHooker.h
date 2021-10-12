#pragma once
#include <type_traits>
#include <QDebug>

template <typename, template<typename> typename, typename = void>
struct Detecter : std::false_type {};

template <typename T, template<typename> typename Op>
struct Detecter<T, Op, std::void_t<Op<T>>> : std::true_type { using type = T; };

template<typename T>
struct QDebugTester
{
	template<typename S>
	static constexpr auto f = static_cast<QDebug& (QDebug::*)(S)>(&QDebug::operator<<);

	template<typename R> using Origin   = decltype(f<R>);
	template<typename R> using Const    = decltype(f<const R>);
	template<typename R> using Ref      = decltype(f<R&>);
	template<typename R> using ConstRef = decltype(f<const R&>);

	template<typename P> using PtrTrait = std::enable_if_t<std::is_pointer_v<P>, void>;
	template<typename P> using ConstPtr = decltype(f<const PtrTrait<P>*>);

	using type =
		std::conditional_t<Detecter<T, Origin>::value,   typename Detecter<T, Origin>::type,
		std::conditional_t<Detecter<T, Const>::value,    typename Detecter<T, Const>::type,
		std::conditional_t<Detecter<T, Ref>::value,      typename Detecter<T, Ref>::type,
		std::conditional_t<Detecter<T, ConstRef>::value, typename Detecter<T, Ref>::type,
		std::conditional_t<Detecter<T, ConstPtr>::value, typename Detecter<T, ConstPtr>::type,
		void>>>>>;

    static constexpr bool hasOp = !std::is_same_v<type, void>;
};

template<typename T>
inline constexpr bool isQDebugSupportedType = QDebugTester<T>::hasOp;
