#pragma once
#include <QDebug>

namespace QDebugTest::Detail
{
// Test T can make 'Op<T>' type
template <typename, template<typename> typename, typename = void>
struct Detector : std::false_type { using type = void; };
template <typename T, template<typename> typename Op>
struct Detector<T, Op, std::void_t<Op<T>>> : std::true_type { using type = T; };

template<typename T, template<typename> typename... Ops>
inline static constexpr bool detectResult = std::conjunction_v<Detector<T, Ops>...>;

struct QDebugDetector
{
	template<typename S>
	static constexpr auto f = static_cast<QDebug& (QDebug::*)(S)>(&QDebug::operator<<);

	template<typename R> using Origin   = decltype(f<R>);
	template<typename R> using Const    = decltype(f<std::add_const_t<R>>);
	template<typename R> using Ref      = decltype(f<std::add_lvalue_reference_t<R>>);
	template<typename R> using ConstRef = decltype(f<std::add_lvalue_reference_t<std::add_const_t<R>>>);

	template<typename P> using PtrTrait = std::enable_if_t<std::is_pointer_v<P>, void>;
	template<typename P> using ConstPtr = decltype(f<const PtrTrait<P>*>);
};

template<typename T>
struct Tester : private QDebugDetector
{
	inline static constexpr bool value =
		detectResult<std::decay_t<T>, Origin, Const, Ref, ConstRef, ConstPtr>;
	constexpr operator bool() const { return value; }
	constexpr bool operator()() const { return value; }
};

template<typename T>
inline constexpr bool isQDebugSupportedType = QDebugTester<T>::value;

template<typename... Ts>
QString debug(Ts&&... ts)
{
	QString buf;
	(QDebug(&buf).nospace().noquote() << ... << ts);
	return buf;
}

}

namespace QDebugTest
{
	using Detail::Tester, Detail::isQDebugSupportedType, Detail::debug;
}
