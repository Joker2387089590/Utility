#pragma once
#include <QDebug>

namespace QDebugTest
{
namespace Detail
{
// Test T can make 'Op<T>' type
template <typename, template<typename> typename, typename = void>
struct Detector : std::false_type { using type = void; };
template <typename T, template<typename> typename Op>
struct Detector<T, Op, std::void_t<Op<T>>> : std::true_type { using type = T; };

// Base of Detector::Op, force '&QDebug::operator<<' to accept 'S' as argument type
template<typename S>
using F = decltype(static_cast<QDebug& (QDebug::*)(S)>(&QDebug::operator<<));

// Detector::Op, decorate F to make a 'QDebug& (QDebug::*)(T const&?)' type
struct Ops
{
	template<typename T> using Origin   = F<T>;
	template<typename T> using Const    = F<T const>;
	template<typename T> using Ref      = F<T &>;
	template<typename T> using ConstRef = F<T const&>;

	// Any pointer except 'const char*' is to 'const void*'
	template<typename P>
	using TraitPtr = std::enable_if_t<std::is_pointer_v<P>, const void*>;
	template<typename P> using ConstPtr = F<TraitPtr<P>>;
};

// Test Ds... one-by-one
template<typename T, template<typename> typename Op,
		 template<typename> typename... Ops>
struct Expander
{
	using D = Detector<T, Op>;
	using type = std::conditional_t<D::value, typename D::type, typename Expander<T, Ops...>::type>;
};

template<typename T, template<typename> typename Op>
struct Expander<T, Op>
{
	using type = typename Detector<T, Op>::type;
};

template<typename T, template<typename> typename... Ds>
using ExType = typename Expander<T, Ds...>::type;
}

template<typename T>
struct Tester : private Detail::Ops
{
	using type = Detail::ExType<std::decay_t<T>, Origin, Const, Ref, ConstRef, ConstPtr>;
	static constexpr bool value = !std::is_same_v<type, void>;
	constexpr operator bool() const { return value; }
	constexpr bool operator()() const { return value; }
};

template<typename T>
inline constexpr bool isQDebugSupportedType = QDebugTest::Tester<T>::value;

template<typename... Ts>
QString debug(Ts&&... ts)
{
	QString buf;
	(QDebug(&buf).nospace().noquote() << ... << ts);
	return buf;
}

}
