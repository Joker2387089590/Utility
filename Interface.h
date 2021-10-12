#pragma once
#include <memory>
#include <type_traits>

namespace _interface_Detail
{
	template<typename Sub, template<typename> typename... Is> class IBase;
	template<template<typename> typename I> class Ptr;

    template<typename Sub, template<typename> typename... Is>
    class IBase : public Is<Sub>... 
	{
		static_assert(std::conjunction_v<std::is_empty<Is<Sub>>...>);
	public:
		using Is<Sub>::Is...;
	};

	inline namespace ChangeType
	{
		template<typename Sub, typename R, typename T, typename... Args>
		auto changeType(R(T::*)(Args...)) -> R(Sub::*)(Args...);

		template<typename Sub, typename R, typename T, typename... Args>
		auto changeType(R(T::*)(Args...) const) -> R(Sub::*)(Args...) const;

		template<auto memFunc, typename Sub>
		using ToSubMem = decltype(changeType<Sub>(memFunc));
	}

	template<template<typename> typename I, typename Sub,
			 std::enable_if_t<std::has_virtual_destructor_v<I<void>>, int> = 0>
	class PtrBase : public I<void>
	{
	public:
		PtrBase(I<Sub>* self) noexcept : obj(static_cast<Sub*>(self)) {}
		~PtrBase() = default;

		PtrBase(const PtrBase& other) : I<void>(other), obj(other.obj) {}
		PtrBase& operator=(const PtrBase&) = default;

		template<auto memFunc, typename... Args>
		decltype(auto) call(ToSubMem<memFunc, Sub> subFunc, Args&&... args)
		{
			return (obj->*subFunc)(std::forward<Args>(args)...);
		}

	private:
		Sub* obj;
	};

	inline namespace GetBase
	{
		template<template<typename> typename I, typename Sub,
				 template<typename> typename... Is>
		struct BaseTrait;

		template<template<typename> typename I, typename Sub>
		struct Match : std::true_type
		{
			using Type = I<Sub>;
			using Ivoid = I<void>;
			using Ptr = typename Type::Ptr;
		};

		template<template<typename> typename I, typename Sub>
		struct BaseTrait<I, Sub> : std::false_type {};

		template<template<typename> typename I, typename Sub,
				 template<typename> typename I0,
				 template<typename> typename... Is>
		struct BaseTrait<I, Sub, I0, Is...> :
			std::conditional_t<std::is_base_of_v<I<Sub>, I0<Sub>>, 
				Match<I0, Sub>, 
				BaseTrait<I, Sub, Is...>>
		{};
	}

	template<template<typename> typename I>
	class Ptr
	{
	public:
		explicit Ptr(std::nullptr_t = nullptr) noexcept {}

		template<typename Sub, template<typename> typename... Is>
		explicit Ptr(IBase<Sub, Is...>* obj) noexcept
		{
			using Trait = BaseTrait<I, Sub, Is...>;
			static_assert(Trait::value);

			using Type = typename Trait::Type;
			using Ivoid = typename Trait::Ivoid;
			static_assert(std::is_base_of_v<I<void>, Ivoid>);

			using IPtr = typename Trait::Ptr;
			p = std::make_shared<IPtr>(obj);
		}

		template<template<typename> typename Ix>
		Ptr& operator=(const Ptr<Ix>& other)
		{

		}

		auto operator->() const { return p.operator->(); }
		auto operator* () const { return p.operator* (); }
		
	private:
		std::shared_ptr<I<void>> p;
	};

} // namespace _interface_Detail

using _interface_Detail::IBase;
using _interface_Detail::PtrBase;
using _interface_Detail::Ptr;
