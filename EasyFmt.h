#pragma once
#include <fmt/format.h>
#include <fmt/color.h>

#if !defined(EASY_FMT_NO_QT) &&  __has_include(<QString>)
#include <QString>
#else
#define EASY_FMT_NO_QT
#endif

inline auto operator""_fmt(const char* str, std::size_t size)
{
    return [f = std::string_view(str, size)](auto&&... args) constexpr {
        if constexpr (sizeof...(args) == 0)
            return f;
        else
			return fmt::vformat(f, fmt::make_format_args(args...));
	};
}

inline auto operator""_print(const char* str, std::size_t size)
{
	return [f = std::string_view(str, size)](auto&&... args) {
		fmt::print(fg(fmt::color::aqua), f, std::forward<decltype(args)>(args)...);
		fmt::print("\n");
	};
}

inline auto operator""_err(const char* str, std::size_t size)
{
	return [f = std::string_view(str, size)](auto&&... args) {
		fmt::print(stderr, fg(fmt::color::crimson), f, std::forward<decltype(args)>(args)...);
		fmt::print(stderr, "\n");
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

template<>
struct fmt::formatter<QByteArray, char> : CFormatter
{
	using CFormatter::parse;
	template <typename FormatContext>
	auto format(const QByteArray& s, FormatContext& context)
	{
		auto view = std::string_view(s.data(), s.size());
		return CFormatter::format(view, context);
	}
};

#endif
