#pragma once
#include <asio/use_awaitable.hpp>
#include <asio/co_spawn.hpp>
#include <asio/as_tuple.hpp>
#include <QMetaObject>
#include <Utility/CallableTrait.h>

namespace CoAsioQt::Details
{
template<typename T>
struct TaskSignature { using type = void(std::exception_ptr, T); };

template<>
struct TaskSignature<void> { using type = void(std::exception_ptr); };

template<typename T>
using TaskSignatureType = typename TaskSignature<T>::type;

template<typename Return, typename C, typename Invoke>
asio::awaitable<Return> runImplWithReturn(C* context, Invoke invoke)
{
    auto executor = co_await asio::this_coro::executor;

    std::optional<Return> result;
    auto wrapper = [context, invoke = std::move(invoke), &result]() mutable -> asio::awaitable<void> {
        auto init = [context, invoke = std::move(invoke), &result](auto handler) mutable {
            QMetaObject::invokeMethod(context, [invoke = std::move(invoke), handler = std::move(handler), &result]() mutable noexcept {
                try {
                    result.emplace(std::move(invoke)());
                    std::move(handler)(std::exception_ptr());
                }
                catch(...) {
                    std::move(handler)(std::current_exception());
                }
            });
        };
        using Token = const asio::use_awaitable_t<>;
        return asio::async_initiate<Token, void(std::exception_ptr)>(std::move(init), asio::use_awaitable);
    };
    auto [ex] = co_await asio::co_spawn(executor, std::move(wrapper), asio::as_tuple(asio::use_awaitable));
    if(ex) std::rethrow_exception(ex);

    co_return std::move(result.value());
}

template<typename C, typename Invoke>
asio::awaitable<void> runImplNoReturn(C* context, Invoke invoke)
{
    auto executor = co_await asio::this_coro::executor;

    auto wrapper = [context, invoke = std::move(invoke)]() mutable -> asio::awaitable<void> {
        auto init = [context, invoke = std::move(invoke)](auto handler) mutable {
            QMetaObject::invokeMethod(context, [invoke = std::move(invoke), handler = std::move(handler)]() mutable noexcept {
                try {
                    std::move(invoke)();
                    std::move(handler)(std::exception_ptr());
                }
                catch(...) {
                    std::move(handler)(std::current_exception());
                }
            });
        };
        using Token = const asio::use_awaitable_t<>;
        return asio::async_initiate<Token, void(std::exception_ptr)>(std::move(init), asio::use_awaitable);
    };
    
    auto [ex] = co_await asio::co_spawn(executor, std::move(wrapper), asio::as_tuple(asio::use_awaitable));
    if(ex) std::rethrow_exception(ex);
}

template<typename C, typename Invoke>
auto runImpl(C* context, Invoke invoke)
{
    using Return = std::invoke_result_t<Invoke>;
    if constexpr (std::is_void_v<Return>)
        return runImplNoReturn(context, std::move(invoke));
    else
        return runImplWithReturn<Return>(context, std::move(invoke));
}

template<typename C, Callables::MemberOf<C> Func, typename... Args>
auto run(C* context, Func func, Args&&... args)
{
    static_assert(std::is_invocable_v<Func, C*, Args&&...>);
    return runImpl(context, [context, func, ...args = std::forward<Args>(args)]() mutable {
        return std::invoke(func, context, std::forward<Args>(args)...);
    });
}

template<typename C, typename Func, typename... Args>
    requires std::is_invocable_v<Func&&, Args&&...>
auto run(C* context, Func&& func, Args&&... args) 
{
    return runImpl(context, [func = std::forward<Func>(func), ...args = std::forward<Args>(args)]() mutable {
        return std::invoke(std::forward<Func>(func), std::forward<Args>(args)...);
    });
}
} // namespace CoAsioQt

namespace CoAsioQt
{
using Details::run;
}
