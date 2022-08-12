#pragma once
#include <fmt/format.h>
#include <fmt/color.h>

#ifndef EASY_FMT_NO_QT
#include <QString>
#endif

inline auto operator""_fmt(const char* str, std::size_t size)
{
    return [f = std::string_view(str, size)](auto&&... args) constexpr {
        if constexpr (sizeof...(args) == 0)
            return f;
        else
            return fmt::vformat(f, fmt::make_args_checked<decltype(args)...>(f, args...));
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

#ifndef EASY_FMT_NO_QT

using U16Formatter = fmt::formatter<std::u16string_view, char16_t>;
using CFormatter = fmt::formatter<std::string_view, char>;
using WFormatter = fmt::formatter<std::wstring_view, wchar_t>;

template<>
struct fmt::formatter<QString, char16_t> : U16Formatter
{
	using U16Formatter::parse;

	template <typename FormatContext>
	auto format(const QString& s, FormatContext& context)
	{
		auto data = reinterpret_cast<const char16_t*>(s.utf16());
		auto size = std::size_t(s.size());
		return U16Formatter::format({data, size}, context);
	}
};

template<>
struct fmt::formatter<QString, char> : CFormatter
{
	using CFormatter::parse;

	template <typename FormatContext>
	auto format(const QString& s, FormatContext& context)
	{
		return CFormatter::format(s.toStdString(), context);
	}
};

template<>
struct fmt::formatter<QString, wchar_t> : WFormatter
{
	using WFormatter::parse;

	template <typename FormatContext>
	auto format(const QString& s, FormatContext& context)
	{
		return WFormatter::format(s.toStdWString(), context);
	}
};

#endif
