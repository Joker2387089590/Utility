#pragma once
#include <memory>
#include <type_traits>

namespace ObjectAddress
{
using AnyPtr = const void*;
using CalcPtr = const std::byte*;

template<typename T>
struct AlignedObject { alignas(T) std::byte buffer[sizeof(T)]; };

template<typename C, typename T>
auto cast(T* addr) noexcept { return std::launder(reinterpret_cast<C>(addr)); }

// 将任意指针转化为可做加减运算的指针
template<typename T>
auto calc(T* addr) noexcept { return cast<CalcPtr>(addr); };

template<typename C, typename T, typename F>
const C* impl(T* mem, F field) noexcept
{
	// 分配一块大小和对齐方式与 ObjectType 相同的空间，即“假对象”
	static constexpr AlignedObject<T> fakeObj{};

	// 假对象的地址，不使用 reinterpret_cast 是因为
	// 非标准布局的类（如有虚函数的类）的指针，不能正确转化为对应地址
	auto* fakeObjAddr = cast<const C*>(std::addressof(fakeObj));

	// 假成员的地址
	auto* fakeMemAddr = std::addressof(fakeObjAddr->*field);

	// 假对象的地址和假成员的地址的偏移量
	auto offset = calc(fakeObjAddr) - calc(fakeMemAddr);

	// 真成员的地址加上偏移量，得到真对象的地址
	return cast<const C*>(calc(mem) + offset);
}

// 重载决定返回值的 const
template<typename T, typename C>
[[nodiscard]] inline auto objAddr(T* mem, T C::* field) noexcept -> C*
{
	return const_cast<C*>(impl<C>(mem, field));
}

template<typename T, typename C>
[[nodiscard]] inline auto objAddr(const T* mem, const T C::* field) noexcept -> const C*
{
	return impl<C>(mem, field);
}

// 参数类型是 const 的，说明是 const C
template<typename T, typename C>
[[nodiscard]] inline auto objAddr(const T* mem, T C::* field) noexcept -> const C*
{
	return impl<C>(mem, field);
}
}

// 从对象成员地址获取对象本身地址
// 用法：Obj* objAddr = objAddr(memAddr, &Obj::memField);
//		memberAddr: 对象成员的地址
//		field: 对象的成员指针
// 注意：如果 memberAddr 并不是真的指向一个 ObjectType 对象，那么是未定义行为
using ObjectAddress::objAddr;
