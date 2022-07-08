#pragma once
#include <utility> // std::forward
#include <tuple>   // std::forward_as_tuple

namespace Lazys
{
/// 惰性求值的生成器
template<typename F>
class Lazy
{
public:
	Lazy(F func) : generator(std::move(func)) {}

    /// 这个对象在遇到需要 func 的返回值时，调用 generator 生成
    operator decltype(auto)() { return generator(); }
    operator decltype(auto)() const { return generator(); }

    /// 主动生成需要的值
    decltype(auto) operator()() { return generator(); }
    decltype(auto) operator()() const { return generator(); }

private:
	F generator;
};

template<typename F> Lazy(F) -> Lazy<F>;

struct CtorTag
{
    explicit constexpr CtorTag() noexcept = default;
} inline constexpr ctor;

// 调用聚合初始化(即 T{args...} )
template<typename T, typename... Args>
auto lazy(Args&&... args)
{
    return Lazy([as = std::forward_as_tuple(std::forward<Args>(args)...)] {
        return std::apply([](auto&&... as) {
            return T{std::forward<Args>(as)...};
        }, as);
    });
}

/// Tag Dispatch，在 try_emplace 时调用括号形式的构造函数
template<typename T, typename... Args>
auto lazy(CtorTag, Args&&... args)
{
    return Lazy([as = std::forward_as_tuple(std::forward<Args>(args)...)] {
        return std::apply([](auto&&... as) {
            return T(std::forward<Args>(as)...);
        }, as);
    });
}
}

// 前向兼容
namespace LazyGenerator = Lazys;
