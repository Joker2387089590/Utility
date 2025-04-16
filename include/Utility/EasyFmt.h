#pragma once
#ifndef EASY_FMT_H
#define EASY_FMT_H

#include <stdexcept>
#include <string_view>
#include <fmt/format.h>
#include <fmt/compile.h>
#include <fmt/xchar.h>
#include <fmt/color.h>

#if !defined(EASY_FMT_NO_QT) &&  __has_include(<QString>)
#    include <QString>
#endif

#include <Utility/Macros.h>
#include <Utility/Literal.h>

namespace EasyFmts
{
/// import fmt literals
inline namespace Literals
{
using namespace fmt::literals;
using fmt::literals::operator""_a;

#if !FMT_USE_NONTYPE_TEMPLATE_ARGS
// NOTE: this use something in fmt::detail
constexpr auto operator""_a(const char16_t* s, std::size_t) -> fmt::detail::udl_arg<char16_t> {
	return {s};
}
#endif
} // namespace Literals

/// fmt::runtime workaround
template<typename Char>
constexpr auto runtime(const Char* str, std::size_t size) noexcept
{
	// fmt::runtime has just overloaded std::string_view/wstring_view, why???
	// https://github.com/fmtlib/fmt/issues/2458
	if constexpr (std::is_same_v<Char, char> || std::is_same_v<Char, wchar_t>)
        return fmt::runtime(fmt::basic_string_view<Char>{str, size});
	else
        return fmt::basic_string_view<Char>(str, size);
}

/// operator""_fmt
#if __cpp_nontype_template_args < 201911L

template<typename Char>
[[nodiscard]] constexpr auto fmtImpl(const Char* str, std::size_t size) noexcept
{
	return [=](auto&&... args) {
		if constexpr (sizeof...(args) == 0)
			return std::basic_string_view<Char>(str, size);
		else
			return fmt::format(runtime(str, size), fwd(args)...);
	};
}

inline namespace Literals
{
[[nodiscard]] constexpr auto operator""_fmt(const char* str, std::size_t size) noexcept { return fmtImpl(str, size); }
[[nodiscard]] constexpr auto operator""_fmt(const wchar_t* str, std::size_t size) noexcept { return fmtImpl(str, size); }
[[nodiscard]] constexpr auto operator""_fmt(const char16_t* str, std::size_t size) noexcept { return fmtImpl(str, size); }
[[nodiscard]] constexpr auto operator""_fmt(const char32_t* str, std::size_t size) noexcept { return fmtImpl(str, size); }

#ifdef __cpp_char8_t
[[nodiscard]] constexpr auto operator""_fmt(const char8_t* str, std::size_t size) noexcept { return fmtImpl(str, size); }
#endif
} // namespace Literals

#else

template<::Literals::StringLiteral s>
constexpr auto fmtImpl = [](auto&&... args) {
	using Char = typename decltype(s)::Char;
	using View = fmt::basic_string_view<Char>;
	constexpr auto view = View(s.data(), s.size());

	if constexpr (sizeof...(args) == 0)
	{
		// we should check the format pattern even if no args
		using Check = fmt::basic_format_string<Char>;
		[[maybe_unused]] constexpr auto check = Check(view);
		return std::basic_string_view<Char>(s.data(), s.size());
	}
	else
		return fmt::format(view, fwd(args)...);
};

inline namespace Literals
{
template<::Literals::StringLiteral s>
[[nodiscard]] constexpr auto operator""_fmt() noexcept { return fmtImpl<s>; }
} // namespace Literals

#endif

/// print
// On Windows, the stdout and stderr is printed by conhost.exe,
// which is the backend of cmd.exe, and it doesn't recognize the ANSI color escape.
// However fmt use ANSI color escape to implement colorful terminal output.
// We can modify the default behavious of conhost by setting Windows Register:
// in `HKCU\Console`, add DWORD `VirtualTerminalLevel` as 1
// Or running command below by Powershell as Admin:
// `Set-ItemProperty HKCU:\Console VirtualTerminalLevel -Type DWORD 1`

#ifdef EASY_FMT_NO_COLOR
#   define EASY_FMT_COLOR(color)
#else
#   define EASY_FMT_COLOR(color) color,
#endif

#if __cpp_nontype_template_args < 201911L
template<typename Color, typename Char>
[[nodiscard]] constexpr auto print(std::FILE* file, Color color, const Char* str, std::size_t size)
{
#ifndef EASY_FMT_NO_CONSOLE
	return [=](auto&&... args) constexpr {
		fmt::print(file, EASY_FMT_COLOR(color) runtime(str, size), fwd(args)...);
		fmt::print("\n");
	};
#else
	return [](auto&&...) {};
#endif
}

inline namespace Literals
{
[[nodiscard]] inline auto operator""_print(const char* str, std::size_t size)
{
	constexpr auto color = fg(fmt::color::UTILITY_EASYFMT_PRINT_COLOR);
	return EasyFmts::print(stdout, color, str, size);
}

[[nodiscard]] inline auto operator""_err(const char* str, std::size_t size)
{
	constexpr auto color = fg(fmt::color::UTILITY_EASYFMT_ERROR_COLOR);
	return EasyFmts::print(stderr, color, str, size);
}

[[nodiscard]] inline auto operator""_fatal(const char* str, std::size_t size)
{
	return [=](auto&&... args) {
		auto msg = std::string(fmtImpl(str, size)(fwd(args)...));
		throw std::runtime_error(msg);
	};
}
} // namespace Literals

#else

template<::Literals::StringLiteral s, typename Color>
[[nodiscard]] constexpr auto print(std::FILE* file, Color color) noexcept
{
	return [file, color](auto&&... args) {
		using Char = typename decltype(s)::Char;
		using View = fmt::basic_string_view<Char>;
		constexpr auto view = View(s.data(), s.size());
		fmt::print(file, EASY_FMT_COLOR(color) view, fwd(args)...);
		fmt::print("\n");
	};
}

inline namespace Literals
{
template<::Literals::StringLiteral s>
[[nodiscard]] inline auto operator""_print()
{
	return EasyFmts::print<s>(stdout, fg(fmt::color::UTILITY_EASYFMT_PRINT_COLOR));
}

template<::Literals::StringLiteral s>
[[nodiscard]] inline auto operator""_err()
{
	return EasyFmts::print<s>(stderr, fg(fmt::color::UTILITY_EASYFMT_ERROR_COLOR));
}

template<::Literals::StringLiteral s>
[[nodiscard]] inline auto operator""_fatal()
{
	return [](auto&&... args) {
		auto msg = std::string(fmtImpl<s>(fwd(args)...));
		throw std::runtime_error(msg);
	};
}
} // namespace Literals
#endif
} // namespace EasyFmts

/// Qt support

#ifndef EASY_FMT_NO_QT
namespace EasyFmts
{
#if __cpp_nontype_template_args < 201911L
// WORKAROUND: MSVC bug
// https://developercommunity.visualstudio.com/t/ICE-when-importing-user-defined-literal/10095949
template<typename Char>
[[nodiscard]] constexpr auto qfmtImpl(const Char* str, std::size_t size)
{
	return [=](auto&&... args) -> QString {
		auto s = fmtImpl(str, size)(fwd(args)...);
		auto rsize = qsizetype(s.size());
		if constexpr (std::is_same_v<Char, char>)
			return QString::fromUtf8(s.data(), rsize);
		else if constexpr (std::is_same_v<Char, wchar_t>)
			return QString::fromWCharArray(s.data(), rsize);
		else if constexpr (std::is_same_v<Char, char16_t>)
			return QString::fromUtf16(s.data(), rsize);
		else if constexpr (std::is_same_v<Char, char32_t>)
			return QString::fromUcs4(s.data(), rsize);
		else
			static_assert(!std::is_same_v<Char, Char>, "Unsupported character type for QString");
	};
}

inline namespace Literals
{
[[nodiscard]] constexpr auto operator""_qfmt(const char* str, std::size_t size)
{
	return qfmtImpl(str, size);
}

// QString is base on UTF-16, this overloaded is faster than the char-version
[[nodiscard]] constexpr auto operator""_qfmt(const char16_t* str, std::size_t size)
{
	return qfmtImpl(str, size);
}

// mix QString with wchar_t seems strange...
auto operator""_qfmt(const wchar_t* str, std::size_t size) = delete;
} // namespace Literals

#else

template<::Literals::StringLiteral l>
[[nodiscard]] constexpr auto qfmtImpl() noexcept
{
	return [](auto&&... args) -> QString {
		using Char = typename decltype(l)::Char;
		auto s = fmtImpl<l>(fwd(args)...);
		if constexpr (std::is_same_v<Char, char>)
			return QString::fromUtf8(s.data(), s.size());
		else if constexpr (std::is_same_v<Char, wchar_t>)
			return QString::fromWCharArray(s.data(), s.size());
		else if constexpr (std::is_same_v<Char, char16_t>)
			return QString::fromUtf16(s.data(), s.size());
		else if constexpr (std::is_same_v<Char, char32_t>)
			return QString::fromUcs4(s.data(), s.size());
		else
			static_assert(!std::is_same_v<Char, Char>, "Unsupported character type for QString");
	};
}

inline namespace Literals
{
template<::Literals::StringLiteral s>
[[nodiscard]] constexpr auto operator""_qfmt() { 
	using Char = typename decltype(s)::Char;
	static_assert(!std::is_same_v<Char, wchar_t>, "mix QString with wchar_t seems strange...");
	return qfmtImpl<s>(); 
}
} // inline namespace Literals

#endif

} // namespace EasyFmts

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
		auto view = std::string_view(s.data(), std::size_t(s.size()));
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
		auto view = std::string_view(s.data(), std::size_t(s.size()));
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
		// QChar is utf-16
		// wchar_t is utf-16 on Windows or usc-4 on other platforms
		static_assert(sizeof(QChar) <= sizeof(wchar_t));

		wchar_t wc = 0;
		[[maybe_unused]] auto size = QStringView(&c, 1).toWCharArray(&wc);
		return Base::format(wc, context);
	}
};

#endif

namespace EasyFmts
{
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
}

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
	return fmt::format(fmt::runtime(r), fwd(args)...);
}
} // namespace EasyFmts

#ifndef EASY_FMT_NO_USING_LITERALS_NAMESPACE
UTILITY_DISABLE_WARNING_PUSH
UTILITY_DISABLE_WARNING_HEADER_HYGIENE
using namespace EasyFmts::Literals;
UTILITY_DISABLE_WARNING_POP
#endif

#include <Utility/MacrosUndef.h>

#endif // EASY_FMT_H
