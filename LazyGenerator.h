#pragma once
#include <tuple>

/// 惰性求值的生成器
template<typename F>
class LazyGenerator
{
public:
	LazyGenerator(F func) : generator(std::move(func)) {}

	/// 这个对象在遇到需要 func 的返回值时，调用 generator 生成
	operator auto() { return generator(); }

private:
	F generator;
};

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

#include <memory>
#include <type_traits>

template<template<typename> typename I, typename Sub>
auto downCast(I<Sub>* p) { return static_cast<Sub*>(p); }

template<template<typename> typename I, typename Sub>
struct PtrHelper : I<void> {};

template<template<typename> typename I, typename Sub>
struct VTable : I<Sub>
{
	typename I<Sub>::Ptr vptr;
};

template<template<typename> typename I>
struct Ptr
{
	Ptr(std::nullptr_t = nullptr) noexcept : p(nullptr) {}

	Ptr(Ptr&&) noexcept = default;
	Ptr& operator=(Ptr&&) noexcept = default;

	template<typename Sub>
	Ptr(VTable<I, Sub>* self) noexcept : p(std::addressof(self->vptr)) {}
	template<typename Sub> Ptr& operator=(I<Sub>* self) { setPtr(self);	}

	Ptr(const Ptr& other) { setPtr(other.p.get()); }

	auto operator->() { return p; }

	I<void>* p;
};

template<typename Sub, template<typename> typename... Is>
struct IBase : public VTable<Is, Sub>...
{

};

template<typename Sub> struct Ix;

template<>
struct Ix<void>
{
	virtual char foo(int) = 0;
	virtual ~Ix() = default;
};

template<typename Sub>
struct Ix
{
	struct Ptr : PtrHelper<Ix, Sub>
	{
		using Helper = PtrHelper<Ix, Sub>;
		using Helper::Helper;
		char foo(int i) override { return downCast(this->self)->foo(i); }
	};

	char foo(int i) = delete;
};

