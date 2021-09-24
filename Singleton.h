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
		class T::ins : public ::Singleton<T> {};
#else
	#error re-define DECLARE_SINGLETON_INS
#endif

#ifndef DECLARE_DLL_SINGLETON_INS
	// Declare that class T::ins is exported from a DLL
	#define DECLARE_DLL_SINGLETON_INS(DLL_MARCO, T) \
		class DLL_MARCO T::ins : public Singleton<T> {};
#else
	#error re-define DECLARE_DLL_SINGLETON_INS
#endif

namespace _singleton_Detail // ugly name to avoid duplicating
{
	// "steal" the constructor of T,
	// this is a non-template class with a template member function
	class SingletonBase
	{
	protected:
		template<typename T, typename... Arg>
		static void construct(T* ins, Arg&&... arg)
			noexcept(std::is_nothrow_constructible_v<T, Arg...>)
		{
			new(ins) T(std::forward<Arg>(arg)...);
		}

		template<typename T>
		static void deconstruct(T* ins) noexcept(std::is_nothrow_destructible_v<T>)
		{
			ins->~T();
		}
	};

	// where to locate the instance
	enum class InstanceLocation : bool
	{
		Heap = false, Stack = true
	};
}

template<typename T,
		 _singleton_Detail::InstanceLocation location = _singleton_Detail::InstanceLocation::Heap>
class Singleton :
	private _singleton_Detail::SingletonBase // hide construct
{
	using IL = _singleton_Detail::InstanceLocation;
	using HeapSpace = struct { char d[sizeof(T)]; };
	using StackSpace = std::aligned_storage_t<sizeof(T), alignof(T)>;
	using Buffer = std::conditional_t<location == IL::Stack, StackSpace, HeapSpace>;

public:
	template<typename... Arg>
	static T* init(Arg&&...arg)
	{
		std::lock_guard locker{ lock() };
		if(exist()) throw std::exception("Initialize an existent object!");

		try
		{
			// allocate at heap so it can be atomatically released.
			if constexpr (location == IL::Heap)
				data().makeBuffer();
			else
				data().flag() = true;

			// construct the instance at the buffer
			construct(instance(), std::forward<Arg>(arg)...);

			return instance();
		}
		catch(...)
		{
			if constexpr (location == IL::Heap)
				data().releaseBuffer();
			else
				data().flag() = false;
			throw;
		}
	}

	template<typename... Arg>
	static T* tryInit(Arg&&... arg) noexcept
	try			{ return init(std::forward<Arg>(arg)...); }
	catch(...)	{ return nullptr; }

	static void destroy()
	{
		std::lock_guard locker{ lock() };
		if constexpr (location == IL::Stack)
			deconstruct(instance());
		else
		{
			auto p = instance();
			detach();
			deconstruct(p);
			data().releaseBuffer();
		}
	}

	static void detach() noexcept(location == IL::Heap)
	{
		if constexpr (location == IL::Stack)
			throw std::exception("detach a object allocated on the stack!");
		else
			data().buffer() = nullptr;
	}

	// make this class like an pointer
	operator T*()	{ return instance(); }
	T* operator->()	{ return instance(); }
	T& operator*()	{ return *instance(); }

	static T* get() noexcept { return exist() ? instance() : nullptr; }

	static bool exist() noexcept
	{
		std::lock_guard locker{ lock() };
		return data().flag();
	}

private:
	static T* instance()
	{
		std::lock_guard locker{ lock() };
		if(!exist()) throw std::exception("Take invalid singleton object!");
		return static_cast<T*>(data().buffer());
	}

	template<IL loc>
	class BufferHelper;

	template<>
	class BufferHelper<IL::Stack>
	{
	public:
		BufferHelper() noexcept : buf{}, f{false} {}
		void* buffer() noexcept { return static_cast<void*>(&buf); }
		bool& flag() noexcept { return f; }
	private:
		StackSpace buf;
		bool f;
	};

	template<>
	class BufferHelper<IL::Heap>
	{
	public:
		BufferHelper() noexcept : bufferPtr{nullptr} {}
		void*& buffer() noexcept { return bufferPtr; }
		bool flag() noexcept { return buffer() != nullptr; }

		void makeBuffer() { buffer() = new char[sizeof(T)]; }
		void releaseBuffer()
		{
			delete[] static_cast<char*>(buffer());
			buffer() = nullptr;
		}
	private:
		void* bufferPtr;
	};

	static auto& data() noexcept
	{
		// template specialisation of BufferHelper
		static BufferHelper<location> helper{};
		return helper;
	}

	static auto& lock() noexcept
	{
		static RecursiveMutex<SpinLock> rMutex;
		return rMutex;
	}
};

template<typename T>
class RaiiSingleton : public T::ins
{
	using Base = typename T::ins;
public:
	template<typename... Arg>
	RaiiSingleton(Arg&&... arg)
	{
		static_assert(std::is_base_of_v<Singleton<T>, typename T::ins>,
					  "T is not a Singleton!");
		Base::init(std::forward<Arg>(arg)...);
	}
	~RaiiSingleton() { Base::destroy(); }

	RaiiSingleton(RaiiSingleton&&) noexcept = default;
	RaiiSingleton(const RaiiSingleton&) = delete;
};
