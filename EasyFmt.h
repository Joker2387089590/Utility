#pragma once
#ifndef EASY_FMT_H
#define EASY_FMT_H

#include <vector>
#include <stdexcept>
#include <string_view>
#include <fmt/format.h>
#include <fmt/xchar.h>
#include <fmt/color.h>

#if !defined(EASY_FMT_NO_QT) &&  __has_include(<QString>)
#	include <QString>
#else
#	define EASY_FMT_NO_QT
#endif

#include "Macros.h"

namespace EasyFmts
{
inline namespace Literals
{
inline constexpr auto vformat = [](auto&& f, auto&&... args) {
	try {
		return fmt::vformat(fwd(f), fmt::make_format_args(args...));
	}
	catch(const std::system_error&) {
		throw;
	}
};

inline constexpr auto print = [](auto&&... args) -> void {
	(void)(args, ...);
#ifndef EASY_FMT_NO_CONSOLE
	try {
		fmt::print(fwd(args)...);
	}
	catch(const std::system_error&) {
		throw;
	}
#endif
};

[[nodiscard]] inline auto operator""_fmt(const char* str, std::size_t size)
{
    return [f = std::string_view(str, size)](auto&&... args) constexpr {
        if constexpr (sizeof...(args) == 0)
            return f;
        else
			return vformat(f, fwd(args)...);
	};
}

[[nodiscard]] inline auto operator""_fmt(const char16_t* str, std::size_t size)
{
    return [f = std::u16string_view(str, size)](auto&&... args) constexpr {
        if constexpr (sizeof...(args) == 0)
            return f;
        else
			return vformat(f, fwd(args)...);
    };
}

[[nodiscard]] inline auto operator""_fmt(const wchar_t* str, std::size_t size)
{
    return [f = std::wstring_view(str, size)](auto&&... args) constexpr {
        if constexpr (sizeof...(args) == 0)
            return f;
        else
			return vformat(f, fwd(args)...);
    };
}

/// On Windows, the stdout and stderr is printed by conhost.exe, 
/// which is the backend of cmd.exe, and it doesn't recognize the ANSI color escape.
/// However fmt use ANSI color escape to implement colorful terminal output.
/// We can modify the default behavious of conhost by setting Windows Register:
/// in `HKCU\Console`, add DWORD `VirtualTerminalLevel` as 1
/// Or running command below by Powershell as Admin:
/// `Set-ItemProperty HKCU:\Console VirtualTerminalLevel -Type DWORD 1`

inline auto operator""_print(const char* str, std::size_t size)
{
	return [f = std::string_view(str, size)](auto&&... args) {
		print(
#ifndef EASY_FMT_NO_COLOR
			fg(fmt::color::aqua),
#endif
			f, fwd(args)...);
		print("\n");
	};
}

inline auto operator""_err(const char* str, std::size_t size)
{
    return [f = std::string_view(str, size)](auto&&... args) {
		print(stderr,
#ifndef EASY_FMT_NO_COLOR
			  fg(fmt::color::crimson),
#endif
			  f, fwd(args)...);
		print(stderr, "\n");
	};
}

inline auto operator""_fatal(const char* str, std::size_t size)
{
	return [f = std::string(str, size)](auto&&... args) {
		if constexpr (sizeof...(args) == 0)
			throw std::runtime_error(f);
		else
			throw std::runtime_error(
				fmt::vformat(f, fmt::make_format_args(fwd(args)...)));
	};
}

#ifndef EASY_FMT_NO_QT
inline auto operator""_qfmt(const char* str, std::size_t size)
{
    return [f = operator""_fmt(str, size)](auto&&... args) -> QString {
        return QString::fromStdString(std::string(f(fwd(args)...)));
    };
}
#endif
} // namespace Literals

#ifndef ARG
#   define ARG(v) fmt::arg(#v, EasyFmts::castEnum(v))
#else
#   error duplicate macro 'ARG' defined!
#endif

template<typename T>
decltype(auto) castEnum(const T& vv)
{
	using type = std::decay_t<T>;
	if constexpr (std::is_enum_v<type>)
		return std::underlying_type_t<type>(vv);
	else
		return vv;
};

// use '@...^' instead of '{...}' as format placeholder
template<typename... Args>
inline auto fjson(std::string_view f, Args&&... args)
{
	enum { Out, InVar } state { Out };
	std::string r;
	std::size_t pre = 0;
	for(std::size_t i = 0; i != f.size(); ++i)
	{
		switch(state)
		{
		case Out:
			switch(f[i])
			{
			case '"':
				i = f.find('"', i + 1);
				if(i == f.npos) throw std::exception();
				continue;
			case '@':
				state = InVar;
				r.append(f.substr(pre, i - pre)).append("{");
				pre = i + 1;
				continue;
			case '{':
				r.append(f.substr(pre, i - pre)).append("{{");
				pre = i + 1;
				continue;
			case '}':
				r.append(f.substr(pre, i - pre)).append("}}");
				pre = i + 1;
				continue;
			default:
				continue;
			}
		case InVar:
			if(f[i] == '^')
			{
				state = Out;
				r.append(f.substr(pre, i - pre)).append("}");
				pre = i + 1;
			}
			continue;
		}
	}
	if(pre < f.size()) r.append(f.substr(pre));
	return fmt::vformat(r, fmt::make_format_args(fwd(args)...));
}
} // namespace EasyFmts

#ifndef EASY_FMT_NO_QT

using U16Formatter = fmt::formatter<std::u16string_view, char16_t>;
using CFormatter = fmt::formatter<std::string_view, char>;
using WFormatter = fmt::formatter<std::wstring_view, wchar_t>;

template<>
struct fmt::formatter<QString, char16_t> : U16Formatter
{
	using U16Formatter::parse;

	template <typename FormatContext>
	auto format(const QString& s, FormatContext& context) const
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
	auto format(const QString& s, FormatContext& context) const
	{
		return CFormatter::format(s.toStdString(), context);
	}
};

template<>
struct fmt::formatter<QString, wchar_t> : WFormatter
{
	using WFormatter::parse;

	template <typename FormatContext>
	auto format(const QString& s, FormatContext& context) const
	{
		return WFormatter::format(s.toStdWString(), context);
	}
};

template<>
struct fmt::formatter<QByteArray, char> : CFormatter
{
	using CFormatter::parse;
	template <typename FormatContext>
	auto format(const QByteArray& s, FormatContext& context) const
	{
		auto view = std::string_view(s.data(), s.size());
		return CFormatter::format(view, context);
	}
};

template<>
struct fmt::formatter<QLatin1String, char> : CFormatter
{
	using CFormatter::parse;
	template <typename FormatContext>
	auto format(const QLatin1String& s, FormatContext& context) const
	{
		auto view = std::string_view(s.data(), s.size());
		return CFormatter::format(view, context);
	}
};

template<>
struct fmt::formatter<QChar, char16_t> : fmt::formatter<char16_t, char16_t>
{
	using Base = fmt::formatter<char16_t, char16_t>;
	using Base::parse;
	template <typename FormatContext>
	auto format(QChar c, FormatContext& context) const
	{
		return Base::format(c.unicode(), context);
	}
};

template<>
struct fmt::formatter<QChar, char> : fmt::formatter<char, char>
{
	using Base = fmt::formatter<char, char>;
	using Base::parse;
	template <typename FormatContext>
	auto format(QChar c, FormatContext& context) const
	{
		return Base::format(c.toLatin1(), context);
	}
};

template<>
struct fmt::formatter<QChar, wchar_t> : fmt::formatter<wchar_t, wchar_t>
{
	using Base = fmt::formatter<wchar_t, wchar_t>;
	using Base::parse;
	template <typename FormatContext>
	auto format(QChar c, FormatContext& context) const
	{
		// why so heavy: converting characters' encode is never trival.
		wchar_t wc = 0;
		QString(c).toWCharArray(&wc);
		return Base::format(wc, context);
	}
};

#endif

#ifndef EASY_FMT_NO_USING_LITERALS_NAMESPACE
using namespace EasyFmts::Literals;
#endif

#include "MacrosUndef.h"

#endif // EASY_FMT_H
