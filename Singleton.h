#pragma once
#include <type_traits>
#include <exception>
#include <mutex>
#include <SpinLock.h>

#ifndef SINGLETON
	// Aggressive interface to assess the singleton instance
	#define SINGLETON					\
		public:							\
			class ins;					\
		private:						\
			friend class _singleton_Detail::SingletonBase;
#else
	#error re-define SINGLETON
#endif

#ifndef DECLARE_SINGLETON_INS
	// Definition of class T::ins
	#define DECLARE_SINGLETON_INS(T) \
		class T::ins : public _singleton_Detail::Singleton<T> {};
#else
	#error re-define DECLARE_SINGLETON_INS
#endif

#ifndef DECLARE_DLL_SINGLETON_INS
	// Declare that class T::ins is exported from a DLL
	#define DECLARE_DLL_SINGLETON_INS(DLL_MARCO, T) \
		class DLL_MARCO T::ins : public _singleton_Detail::Singleton<T> {};
#else
	#error re-define DECLARE_DLL_SINGLETON_INS
#endif

namespace _singleton_Detail // ugly name to avoid duplicating
{
	template<typename T>
	class HeapSpace
	{
	public:
		HeapSpace() noexcept : ptr{nullptr} {}
		void*& buffer() noexcept { return ptr; }
		bool flag() const noexcept { return ptr != nullptr; }

		void* makeBuffer()
		{
			return ptr = new char[sizeof(T)];
		}
		void releaseBuffer()
		{
			delete[] static_cast<char*>(ptr);
			ptr = nullptr;
		}
		
	private:
		void* ptr;
	};

	template<typename T>
	class StackSpace
	{
	public:
		StackSpace() noexcept : buf{}, f{false} {}
		void* buffer() noexcept { return static_cast<void*>(&buf); }
		bool& flag() noexcept { return f; }

	private:
		std::aligned_storage_t<sizeof(T), alignof(T)> buf;
		bool f;
	};

	template<typename T, typename... Args>
	inline constexpr bool wellCtor = std::is_nothrow_constructible_v<T, Args...>;
	template<typename T>
	inline constexpr bool wellDtor = std::is_nothrow_destructible_v<T>;

	// "steal" the constructor of T,
	// this is a non-template class with a template member function
	class SingletonBase
	{
	protected:
		template<typename T, typename... Args>
		static void construct(T* ins, Args&&... args) noexcept(wellCtor<T, Args...>)
		{
			new(ins) T(std::forward<Args>(args)...);
		}

		template<typename T>
		static void deconstruct(T* ins) noexcept(wellDtor<T>)
		{
			ins->~T();
		}
		
		static auto& lock() noexcept
		{
			static RecursiveMutex<SpinLock> mutex;
			return mutex;
		}
	};

	// where to locate the instance
	enum InstanceLocation : bool { Stack = true, Heap = false };

	template<typename T, InstanceLocation location>
	class InstanceInterface : protected SingletonBase
	{
		using Base = SingletonBase;
	protected:
		using Base::construct;
		using Base::deconstruct;
		using Base::lock;

	protected:
		using Buffer = 
			std::conditional_t<!!location, StackSpace<T>, HeapSpace<T>>;

        static auto& data() noexcept
		{
			static Buffer dataBuf{};
			return dataBuf;
		}

		static T* instance()
		{
			std::lock_guard locker{ lock() };
			if(!exist()) throw std::exception("Take invalid singleton object!");
			return static_cast<T*>(data().buffer());
		}

	public:
		static bool exist() noexcept
		{
			std::lock_guard locker{ lock() };
			return data().flag();
		}
		
		static T* get() noexcept
		{
			std::lock_guard locker{ lock() };
			return exist() ? instance() : nullptr;
		}

		// make this class like an pointer
		operator T*()	{ return instance(); }
		T* operator->()	{ return instance(); }
		T& operator*()	{ return *instance(); }
	};

	template<typename T>
	class StackSingleton : public InstanceInterface<T, Stack>
	{
		using Base = InstanceInterface<T, Stack>;
	protected:
		using Base::construct;
		using Base::deconstruct;
		using Base::lock;
		
		using Base::data;
		using Base::instance;

	public:
		using Base::exist;
		using Base::get;

	protected:
		template<typename... Arg>
		static T* init(Arg&&...arg)
		try
		{
			data().flag() = true;
			// construct the instance at the buffer
			construct(instance(), std::forward<Arg>(arg)...);
			return instance();
		}
		catch(...)
		{
			data().flag() = false;
			throw;
		}

		static void destroy()
		{
			deconstruct(instance());
			data().flag() = false;
		}
	};

	template<typename T>
	class HeapSingleton : public InstanceInterface<T, Heap>
	{
		using Base = InstanceInterface<T, Heap>;
	protected:
		using Base::construct;
		using Base::deconstruct;
		using Base::lock;
		
		using Base::data;
		using Base::instance;

	public:
		using Base::exist;
		using Base::get;

	public:
		static void detach() noexcept { data().buffer() = nullptr; }

	protected:
		template<typename... Args>
		static T* init(Args&&... args)
		{
			// allocate at heap so it can be atomatically released.
			data().makeBuffer();
			try
			{
				construct(instance(), std::forward<Args>(args)...);
				return instance();
			}
			catch(...)
			{
				data().releaseBuffer();
				throw;
			}
		}

		static void destroy()
		{
			auto p = instance();
			detach();
			deconstruct(p);
			data().releaseBuffer();
		}
	};

	template<typename T, InstanceLocation location>
	struct LocationTrait;
	template<typename T>
	struct LocationTrait<T, Stack> { using Type = StackSingleton<T>; };
	template<typename T>
	struct LocationTrait<T, Heap> { using Type = HeapSingleton<T>; };

	template<typename T, InstanceLocation location = Heap>
	class Singleton : public LocationTrait<T, location>::Type
	{
		using Base = typename LocationTrait<T, location>::Type;
	private:
		using Base::construct;
		using Base::deconstruct;
		using Base::lock;

		using Base::data;
		using Base::instance;

		using Base::init;
		using Base::destroy;

	public:
		using Base::exist;
		using Base::get;
		using Base::operator *;
		using Base::operator ->;
		using Base::operator T*;

	public:
		template<typename... Args>
		static T* init(Args&&... args)
		{
			std::lock_guard locker{ lock() };
			if(exist()) throw std::exception("Initialize an existent object!");
			return Base::init(std::forward<Args>(args)...);
		}

		template<typename... Args>
		static T* tryInit(Args&&... args) noexcept
		try			{ return init(std::forward<Args>(args)...); }
		catch(...)	{ return nullptr; }

		static void destroy()
		{
			std::lock_guard locker{ Base::lock() };
			Base::destroy();
		}
	};

	template<typename T>
	class RaiiSingleton : public T::ins
	{
		using Base = typename T::ins;
	public:
		template<typename... Args>
		RaiiSingleton(Args&&... args)
		{
			static_assert(std::is_base_of_v<Singleton<T>, Base>,
						  "T is not a Singleton!");
			Base::init(std::forward<Args>(args)...);
		}
		~RaiiSingleton() { Base::destroy(); }

		RaiiSingleton(RaiiSingleton&&) noexcept = default;
		RaiiSingleton(const RaiiSingleton&) = delete;
	};
}

using _singleton_Detail::Singleton;
using _singleton_Detail::RaiiSingleton;
