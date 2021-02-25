/************************* How To Use *****************************
class ST // your singleton type
{
	SINGLETON // put this marco in private! like Q_OBJECT.
		public:
		void func() {}
	~ST()
	{
		// ADD THIS TO Deconstructor
		// if ST is destoried automatically!!!
		// ins::detach();
	}

private:
	ST(int, int) {} // advise to put constructor in private
};
DECLARE_SINGLETON_INS(ST) // out of class!

void example()
{
	// test whether the instance exists.
	bool exist = ST::ins::exist();

	// call "init" BEFORE call "ins"!
	ST::ins::init(1, 2);

	// get instance
	auto p = ST::ins();

	// use like a pointer
	p->func();

	// destroy it manually
	if(exist) ST::ins::destroy();
}
*********************************************************************/
#pragma once
#ifndef SINGLETON_H
#define SINGLETON_H

#include <type_traits>
#include <mutex>
#include <exception>

#ifndef SINGLETON
	#define SINGLETON					\
		public:							\
			class ins;					\
		private:						\
			friend class ::SingletonBase;

	#define DECLARE_SINGLETON_INS(T) \
		class T::ins : public Singleton<T> {};
#else
	#error re-define SINGLETON
#endif

class SingletonBase
{
protected:
	template<typename T, typename... Arg>
	static void construct(T* ins, Arg&&... arg)
		noexcept(std::is_nothrow_constructible_v<T, Arg...>)
	{
		new(ins) T(std::forward<Arg>(arg)...);
	}
};

template<typename T>
class Singleton : public SingletonBase
{
	using Buffer = std::aligned_storage_t<sizeof(T), alignof(T)>;
public:
	using Type = T;

	template<typename... Arg>
	static void init(Arg&&...arg)
	{
		std::lock_guard locker{ lock };
		if(exist()) throw std::exception("Initialize an existent object!");

		try
		{
			buffer = new Buffer{};
			construct(instance(), std::forward<Arg>(arg)...);
		}
		catch(...)
		{
			delete buffer;
			buffer = nullptr;
			throw;
		}
	}

	static void destroy()
	{
		std::lock_guard locker{ lock };
		auto p = instance();
		detach();
		delete p;
	}

	static bool exist() 
	{
		std::lock_guard locker{ lock };
		return buffer != nullptr;
	}

	static void detach() { buffer = nullptr; }

	operator Type*()	{ return instance(); }
	Type* operator->()	{ return instance(); }
	Type& operator*()	{ return *instance(); }

protected:
	static Type* instance()
	{
		std::lock_guard locker{ lock };
		if(!exist()) throw std::exception("Take invalid singleton object!");
		return static_cast<Type*>(static_cast<void*>(buffer));
	}

private:
	inline static Buffer* buffer = nullptr;
	inline static std::recursive_mutex lock;
};

#endif
