#pragma once
#include <fmt/format.h>
#include <fmt/color.h>

inline auto operator""_fmt(const char* str, std::size_t size)
{
	return [f = std::string_view(str, size)](auto&&... args) {
		return fmt::format(f, std::forward<decltype(args)>(args)...);
	};
}

inline auto operator""_print(const char* str, std::size_t size)
{
	return [f = std::string_view(str, size)](auto&&... args) {
		return fmt::print(fg(fmt::color::aqua), f, std::forward<decltype(args)>(args)...);
	};
}

inline auto operator""_err(const char* str, std::size_t size)
{
	return [f = std::string_view(str, size)](auto&&... args) {
		return fmt::print(stderr, fg(fmt::color::crimson), f, std::forward<decltype(args)>(args)...);
	};
}
