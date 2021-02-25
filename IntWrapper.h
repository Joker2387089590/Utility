#include <cstdint>
#include <complex>

namespace IntWrapper
{
    using i8  = std::int8_t;
    using i16 = std::int16_t;
    using i32 = std::int32_t;
    using i64 = std::int64_t;

    using u8  = std::uint8_t;
	using u16 = std::uint16_t;
	using u32 = std::uint32_t;
	using u64 = std::uint64_t;

	using uint = unsigned int;
	using uchar = unsigned char;

    using Port = u16;
}

// Add the marco BEFORE including this file if you need avoid name conflict.
#ifndef DO_NOT_USING_NAMESPACE_INT_WRAPPER
    using namespace IntWrapper;
#endif
