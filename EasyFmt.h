#pragma once
#include <vector>
#include <stdexcept>
#include <fmt/format.h>
#include <fmt/color.h>

#if !defined(EASY_FMT_NO_QT) &&  __has_include(<QString>)
#include <QString>
#else
#define EASY_FMT_NO_QT
#endif

#define fwd(arg) std::forward<decltype(arg)>(arg)

namespace EasyFmts
{
inline namespace Literals
{
[[nodiscard]] inline auto operator""_fmt(const char* str, std::size_t size)
{
    return [f = std::string_view(str, size)](auto&&... args) constexpr {
        if constexpr (sizeof...(args) == 0)
            return f;
        else
			return fmt::vformat(f, fmt::make_format_args(fwd(args)...));
	};
}

inline auto operator""_print(const char* str, std::size_t size)
{
	return [f = std::string_view(str, size)](auto&&... args) {
		fmt::print(fg(fmt::color::aqua), f, fwd(args)...);
		fmt::print("\n");
	};
}

inline auto operator""_err(const char* str, std::size_t size)
{
	return [f = std::string_view(str, size)](auto&&... args) {
		fmt::print(stderr, fg(fmt::color::crimson), f, fwd(args)...);
		fmt::print(stderr, "\n");
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

}

#define ARG(v) fmt::arg(#v, EasyFmts::castEnum(v))

template<typename T>
decltype(auto) castEnum(const T& vv)
{
	using type = std::decay_t<T>;
	if constexpr (std::is_enum_v<type>)
		return std::underlying_type_t<type>(vv);
	else
		return vv;
};

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

template<>
struct fmt::formatter<QLatin1String, char> : CFormatter
{
	using CFormatter::parse;
	template <typename FormatContext>
	auto format(const QLatin1String& s, FormatContext& context)
	{
		auto view = std::string_view(s.data(), s.size());
		return CFormatter::format(view, context);
	}
};

#endif

using namespace EasyFmts::Literals;

#undef fwd
