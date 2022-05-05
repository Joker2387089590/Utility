#pragma once
#include <QDebug>

namespace QDebugTest::Detail
{
// Test T can make 'Op<T>' type
template <typename, template<typename> typename, typename = void>
struct Detector : std::false_type { using type = void; };
template <typename T, template<typename> typename Op>
struct Detector<T, Op, std::void_t<Op<T>>> : std::true_type
{
	using type = typename Op<T>::type;
};

template<typename T,
		 typename = decltype(static_cast<QDebug& (QDebug::*)(T)>(&QDebug::operator<<))>
struct F
{
	using type = T;
};

template<typename T, template<typename> typename... Ops>
using MatchTrait = std::disjunction<Detector<T, Ops>...>;

struct QDebugDetector
{
	template<typename T> using Origin   = F<T>;
	template<typename T> using Const    = F<std::add_const_t<T>>;
	template<typename T> using Ref      = F<std::add_lvalue_reference_t<T>>;
	template<typename T> using ConstRef = F<std::add_lvalue_reference_t<std::add_const_t<T>>>;
};

template<typename T>
struct Tester : private QDebugDetector
{
	using Trait = MatchTrait<std::decay_t<T>, Origin, Const, Ref, ConstRef>;
	using type = typename Trait::type;
	inline static constexpr bool value = Trait::value;
	constexpr operator bool() const noexcept { return value; }
	constexpr bool operator()() const noexcept { return value; }
};

struct QDebugPtrDetector
{
	template<typename T> using Origin = F<std::add_pointer_t<T>>;
	template<typename T> using Const  = F<std::add_pointer_t<std::add_const_t<T>>>;
	template<typename>   using Void   = F<const void*>;
};

template<typename T>
struct Tester<T*> : private QDebugPtrDetector
{
	using Trait = MatchTrait<std::remove_const_t<T>, Origin, Const, Void>;
	using type = typename Trait::type;
	inline static constexpr bool value = Trait::value;
	constexpr operator bool() const noexcept { return value; }
	constexpr bool operator()() const noexcept { return value; }
};

template<typename T> inline constexpr bool isSupported = Tester<T>::value;

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
	using Detail::Tester, Detail::isSupported, Detail::debug;
	static_assert(std::is_same_v<Tester<QString>::type, const QString&>);
	static_assert(std::is_same_v<Tester<char*>::type, const char*>);
	static_assert(std::is_same_v<Tester<int*>::type, const void*>);
}
