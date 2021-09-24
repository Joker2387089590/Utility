#pragma once
#include <type_traits>

// 从对象成员地址获取对象本身地址
// 用法：Obj* objAddr = getObjAddr(memAddr, &Obj::memField);
// memberAddr: 对象成员的地址
// field: 对象的成员指针
// 注意：如果 memberAddr 并不是真的指向一个 ObjectType 对象，那么是未定义行为
template<typename ObjectType, typename MemberType, typename ParaType>
[[nodiscard]] inline auto getObjAddr(ParaType* memberAddr,
									 MemberType ObjectType::* field) noexcept
{
	using namespace std;

	// 判断输入的参数类型是否正确
	static_assert(is_same_v<decay_t<ParaType>, decay_t<MemberType>>,
				  "Parameter type is different from member type!");
	static_assert(!(is_const_v<MemberType> && !is_const_v<ParaType>),
				  "Member type has const type qualifier but parameter type doesn't!");

	using NonTypePtr = const void*;		 // 通用类型的指针
	using ArithmeticalPtr = const char*; // 可运算的指针，void* 做加减法是未定义行为
	using ObjPtr = const ObjectType*;	 // 对象类型的指针

	// 将任意指针转化为可做加减运算的指针
	auto toArithmeticalPtr = [](auto* addr)
	{
		return static_cast<ArithmeticalPtr>(static_cast<NonTypePtr>(addr));
	};

	// 分配一块大小和对齐方式与 ObjectType 相同的空间，即“假对象”
	static constexpr aligned_storage_t<sizeof(ObjectType), alignof(ObjectType)> fakeObj{};

	// 假对象的地址，不使用 reinterpret_cast 是因为
	// 非标准布局的类（如有虚函数的类）的指针，不能正确转化为对应地址
	auto* fakeObjAddr = static_cast<ObjPtr>(static_cast<NonTypePtr>(&fakeObj));

	// 假对象的成员引用
	auto& fakeMember = fakeObjAddr->*field;

	// 假对象的地址和假成员的地址的偏移量
	auto* fakePtr = toArithmeticalPtr(&fakeObj);
	auto* fakeMemPtr = toArithmeticalPtr(&fakeMember);
	std::ptrdiff_t offset = fakePtr - fakeMemPtr;

	// 真成员的地址加上偏移量，得到真对象的地址
	auto* realMemPtr = toArithmeticalPtr(memberAddr);
	auto realObjAddr = static_cast<NonTypePtr>(realMemPtr + offset);

	// 返回的对象的 const 属性判断：
	// 如果成员本身是 const 的，那么无法判断，出于安全返回 const ObjectType；
	// 成员本身不是 const 的，但参数类型是 const 的，说明是 const ObjectType。
	using ReturnType =
		conditional_t<is_const_v<MemberType>, add_const_t<ObjectType>,
					  conditional_t<is_const_v<ParaType>, add_const_t<ObjectType>,
									ObjectType>>;

	// 转换为真对象的地址
	return static_cast<ReturnType*>(const_cast<void*>(realObjAddr));
}
