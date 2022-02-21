#pragma once
#include <climits>      // CHAR_BIT
#include <cstdint>      // std::int*_t
#include <limits>       // std::numeric_limits
#include <type_traits>

namespace IntWrapper
{
	inline namespace ShortName
	{
		using i8  = std::int8_t;
		using i16 = std::int16_t;
		using i32 = std::int32_t;
		using i64 = std::int64_t;

		using u8  = std::uint8_t;
		using u16 = std::uint16_t;
		using u32 = std::uint32_t;
		using u64 = std::uint64_t;

		using uint  = unsigned int;
		using uchar = unsigned char;

		using lint  = std::intmax_t;	// longest int
		using luint = std::uintmax_t;	// longest uint
		using sint  = signed char;		// shortest int
        using suint = unsigned char;	// shortest uint

        using pint = std::uintptr_t;	// pointer length
        using oint = std::ptrdiff_t;	// pointer offset

		namespace Detail
		{
			template<std::size_t size>
            constexpr void bitTester() {}

			template<std::size_t size, typename T, typename... Ts>
            constexpr auto bitTester()
			{
				if constexpr (sizeof(T) == size)
					return T{};
				else
                    return bitTester<size, Ts...>;
			}

            template<std::size_t bit, typename... Ts>
            using TypeHasBit = decltype(bitTester<bit / CHAR_BIT, Ts...>());
		}

		template<std::size_t bit>
        using Fs = Detail::TypeHasBit<bit, float, double, long double>;

		using f32 = Fs<32>;
		using f64 = Fs<64>;
		using f80 = Fs<80>;
	}

	inline namespace BitTemplate
	{
		namespace Detail
		{
			template<std::size_t i>
			struct BitInt 
			{ 
				static_assert(i % CHAR_BIT != 0, "Error bit!");
				static_assert(i / CHAR_BIT > sizeof(lint), "Too big!");
			};

			#define SS(bit)	\
			template<> struct BitInt<bit> { using TI = i##bit; using TU = u##bit; }
			SS(8); SS(16); SS(32); SS(64);
			#undef SS
		}

		template<std::size_t i> using IntHasBit = typename Detail::BitInt<i>::TI;
		template<std::size_t i> using UIntHasBit = typename Detail::BitInt<i>::TU;

		template<std::size_t i> using IntHasByte = IntHasBit<i * CHAR_BIT>;
		template<std::size_t i> using UIntHasByte = UIntHasBit<i * CHAR_BIT>;
	}

	inline namespace PtrCast
	{
		template<typename T>
		[[nodiscard]] inline pint ptrToInt(T* ptr) noexcept
		{ 
			return reinterpret_cast<pint>(ptr);
		}

		namespace Detail
		{
			template<typename F>
			using RawFunc = std::remove_pointer_t<std::decay_t<F>>;

			template<typename F>
			inline constexpr bool isFunction = std::is_function_v<RawFunc<F>>;
		}

		// Cast function pointer to an unsigned int.
		template<typename Fn>
		[[nodiscard]] inline pint funcToInt(Fn* f) noexcept
		{
			static_assert(std::is_function_v<Fn>, "f is not a function!");
			auto tmpPtr = reinterpret_cast<void**>(&f);
			return reinterpret_cast<pint>(*tmpPtr);
		}

		template<typename Fn,
				 std::enable_if_t<Detail::isFunction<Fn>, int> = 0>
		[[nodiscard]] auto intToFunc(pint i) noexcept
		{
			return reinterpret_cast<Detail::RawFunc<Fn>*>(i);
		}

		template<auto f>
		[[nodiscard]] auto intToFunc(pint i) noexcept
		{
			return intToFunc<decltype(f)>(i);
		}
	}

	namespace Limit
	{
		template<typename T>
		inline constexpr T max = std::numeric_limits<T>::max();

		template<typename T>
		inline constexpr T min = std::numeric_limits<T>::min();
	}

	inline namespace Literal
	{
		constexpr auto operator""_sz(unsigned long long int z) { return z; }
	}

	using Port = u16; // TCP network port
}

// To avoid name conflicting, add the marco BEFORE including this header.
#ifndef DO_NOT_USING_NAMESPACE_INTWRAPPER
	using namespace ::IntWrapper;
#endif
