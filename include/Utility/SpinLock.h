#pragma once
#include <atomic>
#include <thread>
#if defined (_MSC_VER)
	#include <intrin.h>
#endif

class SpinLock
{
public:
	inline void lock() noexcept
	{
		for (;;)
		{
			// Optimistically assume the lock is free on the first try
			if (!flag.exchange(true, std::memory_order_acquire)) return;

			// Wait for lock to be released without generating cache misses
			while (flag.load(std::memory_order_relaxed))
			{
				// Issue X86 PAUSE or ARM YIELD instruction
				// to reduce contention between hyper-threads
			#if defined (_MSC_VER)
				_mm_pause();
			#elif defined (__GNUG__) || defined (__clang__)
				__builtin_ia32_pause();
			#endif
			}
		}
	}

	inline bool try_lock() noexcept
	{
		// First do a relaxed load to check if lock is free in order to prevent
		// unnecessary cache misses if someone does while(!try_lock())
		return !flag.load(std::memory_order_relaxed) &&
			   !flag.exchange(true, std::memory_order_acquire);
	}

	inline void unlock() noexcept
	{
		flag.store(false, std::memory_order_release);
	}

private:
	std::atomic_bool flag{ false };
};

// Wrap any mutex to recursive mutex
template<typename Mutex>
class RecursiveMutex : public Mutex
{
public:
	void lock() noexcept(std::is_nothrow_invocable_v<decltype(&Mutex::lock)>)
	{
		auto thisId = std::this_thread::get_id();
		if(m_owner == thisId)
		{
			// recursive locking
			m_count++;
		}
		else
		{
			// normal locking
			Mutex::lock();
			m_owner = thisId;
			m_count = 1;
		}
	}

	void unlock() noexcept(std::is_nothrow_invocable_v<decltype(&Mutex::unlock)>)
	{
		if(m_count > 1)
		{
			// recursive unlocking
			m_count--;
		}
		else
		{
			// normal unlocking
			m_owner = std::thread::id{};
			m_count = 0;
			Mutex::unlock();
		}
	}

private:
	std::atomic<std::thread::id> m_owner;
	int m_count;
};
