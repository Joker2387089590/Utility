#pragma once
#include <Utility/TypeList.h>
#include <Utility/CallableTrait.h>

// 参考 https://github.com/microsoft/proxy
namespace Proxys::Detail
{
/// forward declaration
template<typename... Ds> struct Facade;
template<typename... Ds> class ProxyImpl;
template<typename... Ds> class UniqueProxyImpl;
template<template<typename...> typename P, typename F> struct FacadeTrait;

template<typename F>
using UniqueProxy = typename FacadeTrait<UniqueProxyImpl, F>::Proxys;

template<typename F>
using Proxy = typename FacadeTrait<ProxyImpl, F>::Proxys;

/// dispatch
template<typename F> struct Dispatch;
/**
 * @brief Dispatch 结构体模板定义了一个占位结构体，用于表示调度器。
 *  它接受一个函数类型作为模板参数。
 *
 * @tparam R 
 * @tparam As 
 */
template<typename R, typename... As>
struct Dispatch<R(As...)>
{
    // 使用 Call 别名定义了一个 Callable，
    // 它是一个模板类型，用于表示具有特定返回类型和参数类型的可调用对象。
    using Call = Callables::Callable<R(As...)>;
};

/**
 * @brief isDispatch 函数的重载，用于判断给定类型是否为 Dispatch 的实例。
 * 如果是 Dispatch 的实例，则返回 std::true_type，否则返回 std::false_type。
 *
 * @tparam F 
 * @return std::true_type 
 */
template<typename F> auto isDispatch(const Dispatch<F>&) -> std::true_type;
auto isDispatch(...) -> std::false_type;

/**
 * @brief IsDispatch 是一个模板别名，用于根据给定类型判断是否为 Dispatch 的实例。
 * 
 * @tparam T 
 */
template<typename T> using IsDispatch = decltype(isDispatch(std::declval<T>()));

/// facade
/// Wrap 结构模板，用于包装成员函数指针以便于调用
template<typename T, typename D, typename Invoker> struct Wrap;

/// Wrap 结构模板的特化版本，当调用类型为 R(As...) 时
template<typename T, typename D, typename R, typename... As>
struct Wrap<T, D, R(As...)>
{
    /**
     * @brief 实现函数，用于调用成员函数并返回结果
     * 
     * @param object 指向对象的指针
     * @param args 成员函数的参数列表
     * @return R 调用结果
     */
    static constexpr R impl(void* object, As... args)
    {
        return std::invoke(D{}, *static_cast<T*>(object), std::forward<As>(args)...);
    }
};
/// wrap 模板变量，用于存储 Wrap 结构模板的实例化结果，即成员函数指针
template<typename T, typename D>
constexpr auto* wrap = Wrap<T, D, typename D::Call::Invoker>::impl;

/// Unwrap 结构模板，用于解包成员函数指针类型
template<typename D>
struct Unwrap
{
    /// Impl 结构模板，用于存储解包后的成员函数指针类型
    template<typename R, typename... As>
    struct Impl { using type = R(*)(void*, As...); };

    /// type 成员类型，存储解包后的成员函数指针类型
    using type =
        typename D::Call::
        template Expand<Impl>::
        type;
};

/**
 * @brief Facade 结构体模板，用于生成多个分发器的外观。
 * 
 * @tparam Ds 分发器类型参数包。
 */
template<typename... Ds>
struct Facade
{
    // 定义类型别名，将参数包 Ds 转换为类型列表 DList
    using DList = Types::TList<Ds...>;
    // 静态断言，检查所有分发器类型是否满足 IsDispatch 条件
    static_assert(DList::template ifAll<IsDispatch>);
    // 静态断言，检查参数包中是否存在重复类型
    static_assert(!DList::repeated);

    // 定义 VTable 类型，存储各个分发器的解包后的类型
    using VTable = std::tuple<typename Unwrap<Ds>::type...>;

    // 定义静态 constexpr 变量 vtable，存储各个分发器的封装后的类型
    template<typename T>
    inline static constexpr VTable vtable{ wrap<T, Ds>... };
};

/// 推导函数模板，返回值类型为 void
auto toFacade(...) -> void;
/// 推导函数模板的重载版本，接受 Facade<Ds...> 类型的参数，返回相同类型的 Facade 对象
template<typename... Ds> auto toFacade(const Facade<Ds...>&) -> Facade<Ds...>;

/// FacadeTrait 结构体模板，接受模板模板参数 P 和类型参数 F
/// 通过 toFacade 函数和 decltype 推导出 P<Ds...> 类型，并将其定义为 Proxys 的别名
template<template<typename...> typename P, typename F>
struct FacadeTrait : public FacadeTrait<P, decltype(toFacade(std::declval<F>()))> {};

/// FacadeTrait 的特化版本，当 F 类型为 Facade<Ds...> 时，将 Proxys 定义为 P<Ds...> 的别名
template<template<typename...> typename P, typename... Ds>
struct FacadeTrait<P, Facade<Ds...>> { using Proxys = P<Ds...>; };

/// proxy impl
template<typename... Ds>
class ProxyImpl
{
    using Facades = Facade<Ds...>;
    using VTable = const typename Facades::VTable;
    template<typename... D> friend class UniqueProxyImpl;
public:
    constexpr ProxyImpl() noexcept : object(nullptr), vptr(nullptr) {}

    constexpr ProxyImpl(std::nullptr_t) noexcept : ProxyImpl() {}

    template<typename T>
    constexpr ProxyImpl(T* object) noexcept :
        object(object),
        vptr(std::addressof(Facades::template vtable<T>))
    {}

    template<typename T>
    constexpr ProxyImpl& operator=(T* object) & noexcept
    {
        this->object = object;
        this->vptr = std::addressof(Facades::template vtable<T>);
        return *this;
    }

public:
    constexpr ProxyImpl(ProxyImpl&&) noexcept = default;
    constexpr ProxyImpl& operator=(ProxyImpl&&) & noexcept = default;
    constexpr ProxyImpl(const ProxyImpl&) noexcept = default;
    constexpr ProxyImpl& operator=(const ProxyImpl&) & noexcept = default;
    ~ProxyImpl() noexcept = default;

public:
    template<typename D, typename... As>
    decltype(auto) invoke(As&&... args) const
    {
        constexpr auto index = Types::TList<Ds...>::template find<D>();
        static_assert(index != -1);
        return std::get<index>(*vptr)(object, std::forward<As>(args)...);
    }

    constexpr operator bool() const noexcept { return !!object; }

    template<typename T>
    constexpr T* as() const noexcept { return static_cast<T*>(object); }

private:
    void* object;
    VTable* vptr;
};

/// unique proxy impl

struct Destroy : Dispatch<void()>
{
    template<typename T> void operator()(T& obj) { delete &obj; }
};

template<typename... Ds>
class UniqueProxyImpl : public ProxyImpl<Destroy, Ds...>
{
    using Base = ProxyImpl<Destroy, Ds...>;
    template<typename... D> friend class UniqueProxyImpl;
public:
    constexpr UniqueProxyImpl() noexcept : Base() {}
    constexpr UniqueProxyImpl(std::nullptr_t) noexcept : Base() {}

public: // move only
    UniqueProxyImpl(const UniqueProxyImpl&) = delete;
    UniqueProxyImpl& operator=(const UniqueProxyImpl&) & = delete;

    UniqueProxyImpl(UniqueProxyImpl&& other) noexcept : UniqueProxyImpl()
    {
        *this = std::move(other);
    }

    UniqueProxyImpl& operator=(UniqueProxyImpl&& other) & noexcept
    {
        std::swap(this->object, other.object);
        std::swap(this->vptr, other.vptr);
        return *this;
    }

    ~UniqueProxyImpl() { if(this->object) this->template invoke<Destroy>(); }

public: // non-trivial contructors
    // add explicit
    template<typename T>
    explicit constexpr UniqueProxyImpl(T* object) : Base(object) {}

    // the deleter cannot be created
    template<typename... D>
    UniqueProxyImpl(ProxyImpl<D...> proxy) = delete;

public: // to normal proxy
    operator ProxyImpl<Ds...>() const { return static_cast<const Base&>(*this); }

public: // Base function
    using Base::invoke;
    using Base::operator bool;
    using Base::as;
};
} // namespace Proxys::Detail

namespace Proxys
{
using Detail::Dispatch;
using Detail::Facade;
using Detail::Proxy;
using Detail::UniqueProxy;
}
