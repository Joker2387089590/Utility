#pragma once
#ifndef EASY_FMT_H
#define EASY_FMT_H

#include <stdexcept>
#include <string_view>
#include <fmt/format.h>
#include <fmt/compile.h>
#include <fmt/xchar.h>
#include <fmt/color.h>
#include <fmt/std.h>

#if !defined(EASY_FMT_NO_QT)
#    include <Utility/FmtQt.h>
#endif

#include <Utility/Macros.h>
#include <Utility/Literal.h>

#if __cpp_nontype_template_args >= 201911L
#	 define UTILITY_EASY_FMT_OLD_LITERAL 0
#elif __clang__ && __clang_major__ >= 12 && __cplusplus >= 202002L
#	 define UTILITY_EASY_FMT_OLD_LITERAL 0
#else
#	 define UTILITY_EASY_FMT_OLD_LITERAL 1
#endif

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
		return fmt::runtime({str, size});
	else
		return fmt::basic_string_view<Char>(str, size);
}

/// operator""_fmt
#if UTILITY_EASY_FMT_OLD_LITERAL

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

#else // UTILITY_EASY_FMT_OLD_LITERAL

template<::Literals::StringLiteral s>
inline constexpr auto fmtImpl = [](auto&&... args) {
	using Char = typename decltype(s)::Char;
	using View = fmt::basic_string_view<Char>;
	constexpr auto view = View{s.data(), s.size()};

	// we should check the format pattern even if no args
	using Check = fmt::basic_format_string<Char, std::decay_t<decltype(args)>...>;
	[[maybe_unused]] constexpr auto check = Check{view};
	
	if constexpr (sizeof...(args) == 0)
		return std::basic_string_view<Char>(s.data(), s.size());
	else
		return fmt::format(view, std::forward<decltype(args)>(args)...);
};

inline namespace Literals
{
template<::Literals::StringLiteral s>
[[nodiscard]] constexpr auto operator""_fmt() noexcept { return fmtImpl<s>; }
} // namespace Literals

#endif // UTILITY_EASY_FMT_OLD_LITERAL

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

template<typename Char> struct NewLine;
template<> struct NewLine<char> { static constexpr auto value = '\n'; };
template<> struct NewLine<wchar_t> { static constexpr auto value = L'\n'; };
#ifdef __cpp_char8_t
template<> struct NewLine<char8_t> { static constexpr auto value = u8'\n'; };
#endif
template<> struct NewLine<char16_t> { static constexpr auto value = u'\n'; };
template<> struct NewLine<char32_t> { static constexpr auto value = U'\n'; };
template<typename Char> inline constexpr Char newLine = NewLine<Char>::value;

#if UTILITY_EASY_FMT_OLD_LITERAL
template<typename Color, typename Char>
[[nodiscard]] constexpr auto print(std::FILE* file, Color color, const Char* str, std::size_t size)
{
#ifndef EASY_FMT_NO_CONSOLE
	return [=](auto&&... args) constexpr {
#ifdef __cpp_lib_smart_ptr_for_overwrite
		auto buf = std::make_unique_for_overwrite<Char[]>(size + 1);
#else
		auto buf = std::make_unique<Char[]>(size + 1);
#endif
		auto back = std::copy(str, str + size, buf.get());
		*back = newLine<Char>;
		fmt::print(file, EASY_FMT_COLOR(color) runtime(buf.get(), size + 1), fwd(args)...);
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

#else // UTILITY_EASY_FMT_OLD_LITERAL

template<typename Char, std::size_t N>
consteval auto patternWithNewLine(const ::Literals::StringLiteral<Char, N>& s)
{
	::Literals::StringLiteral<Char, N + 1> p;
	std::copy(std::begin(s.value), std::end(s.value), std::begin(p.value));
	p.value[N - 1] = newLine<Char>;
	return p;
}

template<::Literals::StringLiteral s, typename Color>
[[nodiscard]] constexpr auto print(std::FILE* file, Color color) noexcept
{
	return [file, color](auto&&... args) {
		using Char = typename decltype(s)::Char;
		using View = fmt::basic_string_view<Char>;
		constexpr auto view = View(s.data(), s.size());
		fmt::print(file, EASY_FMT_COLOR(color) view, fwd(args)...);
	};
}

inline namespace Literals
{
template<::Literals::StringLiteral s>
[[nodiscard]] inline auto operator""_print()
{
	constexpr auto pattern = patternWithNewLine(s);
	return EasyFmts::print<pattern>(stdout, fg(fmt::color::UTILITY_EASYFMT_PRINT_COLOR));
}

template<::Literals::StringLiteral s>
[[nodiscard]] inline auto operator""_err()
{
	constexpr auto pattern = patternWithNewLine(s);
	return EasyFmts::print<pattern>(stderr, fg(fmt::color::UTILITY_EASYFMT_ERROR_COLOR));
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

#endif // UTILITY_EASY_FMT_OLD_LITERAL

} // namespace EasyFmts

/// Qt support

#ifndef EASY_FMT_NO_QT
namespace EasyFmts
{
#if UTILITY_EASY_FMT_OLD_LITERAL
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

#else // UTILITY_EASY_FMT_OLD_LITERAL

template<::Literals::StringLiteral l>
inline constexpr auto qfmtImpl = [](auto&&... args) -> QString {
	using Char = typename decltype(l)::Char;
	auto s = fmtImpl<l>(fwd(args)...);
	if constexpr (std::is_same_v<Char, char>)
		return QString::fromUtf8(s.data(), qsizetype(s.size()));
	else if constexpr (std::is_same_v<Char, wchar_t>)
		return QString::fromWCharArray(s.data(), qsizetype(s.size()));
	else if constexpr (std::is_same_v<Char, char16_t>)
		return QString::fromUtf16(s.data(), qsizetype(s.size()));
	else if constexpr (std::is_same_v<Char, char32_t>)
		return QString::fromUcs4(s.data(), qsizetype(s.size()));
	else
		static_assert(!std::is_same_v<Char, Char>, "Unsupported character type for QString");
};

inline namespace Literals
{
template<::Literals::StringLiteral s>
[[nodiscard]] constexpr auto operator""_qfmt() { 
	using Char = typename decltype(s)::Char;
	static_assert(!std::is_same_v<Char, wchar_t>, "mix QString with wchar_t seems strange...");
	return qfmtImpl<s>; 
}
} // inline namespace Literals

#endif // UTILITY_EASY_FMT_OLD_LITERAL

} // namespace EasyFmts

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
