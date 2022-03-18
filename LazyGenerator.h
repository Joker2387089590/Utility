#pragma once
#include <tuple>

namespace LazyGenerator
{
/// 惰性求值的生成器
template<typename F>
class Lazy
{
public:
	Lazy(F func) : generator(std::move(func)) {}

	/// 这个对象在遇到需要 func 的返回值时，调用 generator 生成
	operator auto() { return generator(); }

private:
	F generator;
};

template<typename F> Lazy(F) -> Lazy<F>;

template<typename T, typename... Args>
auto lazy(Args&&... args)
{
	return Lazy([&] {
		if constexpr (std::is_default_constructible_v<T>)
			return T{std::forward<Args>(args)...};
		else
			return T(std::forward<Args>(args)...);
	});
}

struct EmplaceTag {};
inline constexpr EmplaceTag emplaceTag;

template<typename T>
struct EmplaceHelper : public T
{
	/// Tag Dispatch，在 try_emplace 时调用聚合初始化(即 T{args...} )
	/// 一般用在 T 是没有构造函数的结构体的情形
	template<typename... Args>
	EmplaceHelper(EmplaceTag, Args&&... args) :
		T{ std::forward<Args>(args)... }
	{}

	using T::T;
};

template<typename T>
struct TryEmplaceHelper : public T
{
	/// Tag Dispatch，在 try_emplace 时调用聚合初始化(即 T{args...} )
	/// 一般用在 T 是没有构造函数的结构体的情形
	template<typename... Args>
	TryEmplaceHelper(EmplaceTag, Args&&... args) :
		T{ std::forward<Args>(args)... }
	{}

	TryEmplaceHelper(const TryEmplaceHelper&) = delete;
	TryEmplaceHelper& operator=(const TryEmplaceHelper&) = delete;
};
}
