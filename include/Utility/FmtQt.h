#pragma once
#include <QString>
#include <QStringView>
#include <QByteArray>
#include <QLatin1String>
#include <QChar>
#include <QList>
#include <fmt/format.h>
#include <fmt/compile.h>
#include <fmt/xchar.h>

/// QStringView
template<typename Char>
struct fmt::formatter<QStringView, Char> : fmt::formatter<std::basic_string_view<Char>, Char>
{
	using Base = fmt::formatter<std::basic_string_view<Char>, Char>;
	using Base::parse;

	template <typename FormatContext>
	auto format(QStringView s, FormatContext& context) const
	{
        if constexpr (std::is_same_v<Char, char>)
        {
            auto utf8 = s.toUtf8();
            auto data = reinterpret_cast<const char*>(utf8.data());
            auto size = std::size_t(utf8.size());
            return Base::format({data, size}, context);
        }
		else if constexpr (std::is_same_v<Char, wchar_t>)
		{
			// QChar is utf-16
			// wchar_t is utf-16 on Windows or usc-4(utf-32) on other platforms
			static_assert(sizeof(QChar) <= sizeof(wchar_t));
			fmt::basic_memory_buffer<wchar_t> buffer;
			buffer.resize(std::size_t(s.size()));
			auto size = s.toWCharArray(buffer.data());
			return Base::format({buffer.data(), std::size_t(size)}, context);
		}
#ifdef __cpp_char8_t
		else if constexpr (std::is_same_v<Char, char8_t>)
		{
			auto utf8 = s.toUtf8();
			auto data = reinterpret_cast<const char8_t*>(utf8.data());
			auto size = std::size_t(s.size());
			return Base::format({data, size}, context);
		}
#endif
        else if constexpr (std::is_same_v<Char, char16_t>)
        {
            auto data = reinterpret_cast<const char16_t*>(s.utf16());
            auto size = std::size_t(s.size());
            return Base::format({data, size}, context);
		}
        else if constexpr (std::is_same_v<Char, char32_t>)
        {
			const auto utf32 = s.toUcs4();
			auto data = reinterpret_cast<const char32_t*>(utf32.data());
            auto size = std::size_t(s.size());
			return Base::format({data, size}, context);
		}
        else
        {
            static_assert(!std::is_same_v<Char, Char>, "Unsupported character type for QString");
        }
	}
};

template<typename Char>
struct fmt::range_format_kind<QStringView, Char> :
	std::integral_constant<fmt::range_format, range_format::disabled>
{};

/// QString
template<typename Char>
struct fmt::formatter<QString, Char> : fmt::formatter<QStringView, Char>
{
	using Base = fmt::formatter<QStringView, Char>;
	using Base::parse;

	template <typename FormatContext>
	auto format(const QString& s, FormatContext& context) const
	{
		return Base::format(QStringView(s), context);
	}
};

template<typename Char>
struct fmt::range_format_kind<QString, Char> :
	std::integral_constant<fmt::range_format, range_format::disabled>
{};

/// QByteArrayView
// treat QByteArray as a utf-8 string
template<>
struct fmt::formatter<QByteArrayView, char> : fmt::formatter<std::string_view, char>
{
    using Base = fmt::formatter<std::string_view, char>;
    using Base::parse;

    template <typename FormatContext>
	auto format(QByteArrayView view, FormatContext& context) const
	{
		return Base::format(std::string_view(view), context);
    }
};

#ifdef __cpp_char8_t
template<>
struct fmt::formatter<QByteArrayView, char8_t> : fmt::formatter<std::u8string_view, char8_t>
{
	using Base = fmt::formatter<std::u8string_view, char8_t>;
	using Base::parse;

	template <typename FormatContext>
	auto format(QByteArrayView view, FormatContext& context) const
	{
		auto data = reinterpret_cast<const char8_t*>(view.data());
		return Base::format({data, std::size_t(view.size())}, context);
	}
};
#endif

template<typename Char>
struct fmt::range_format_kind<QByteArrayView, Char> :
	std::integral_constant<fmt::range_format, range_format::disabled>
{};

/// QByteArray
template<typename Char>
struct fmt::formatter<QByteArray, Char> : fmt::formatter<QByteArrayView, Char>
{
	using Base = fmt::formatter<QByteArrayView, Char>;
	using Base::parse;

	template <typename FormatContext>
	auto format(const QByteArray& s, FormatContext& context) const
	{
		return Base::format(QByteArrayView(s), context);
	}
};

template<typename Char>
struct fmt::range_format_kind<QByteArray, Char> :
	std::integral_constant<fmt::range_format, range_format::disabled>
{};

/// QLatinString
// latin string is conpatible with utf-8 string, so we can treat it as a utf-8 string
template<>
struct fmt::formatter<QLatin1String, char> : fmt::formatter<std::string_view, char>
{
    using Base = fmt::formatter<std::string_view, char>;
	using Base::parse;

	template <typename FormatContext>
	auto format(const QLatin1String& s, FormatContext& context) const
	{
		auto view = std::string_view(s.data(), std::size_t(s.size()));
		return Base::format(view, context);
	}
};

template<typename Char>
struct fmt::range_format_kind<QLatin1String, Char> :
	std::integral_constant<fmt::range_format, range_format::disabled>
{};

template<typename Char>
struct fmt::formatter<QChar, Char> : fmt::formatter<Char, Char>
{
	using Base = fmt::formatter<Char, Char>;
	using Base::parse;

	template <typename FormatContext>
	auto format(QChar c, FormatContext& context) const
	{
        if constexpr (std::is_same_v<Char, char>)
        {
            // NOTE: 
			// utf-16 is a variable-length encoding,
			// its code point consists of one or two code unit,
			// QChar is a code unit of utf-16, not a code point,
			// so it can not be converted to utf-8 code units directly,
            // hence, we convert it to ASCII/latin.
            return Base::format(c.toLatin1(), context);
        }
        else if constexpr (std::is_same_v<Char, char16_t>)
        {
            return Base::format(c.unicode(), context);
        }
        else if constexpr (std::is_same_v<Char, wchar_t>)
        {
            wchar_t wc = 0;
            [[maybe_unused]] auto size = QStringView(&c, 1).toWCharArray(&wc);
            return Base::format(wc, context);
        }
        else if constexpr (std::is_same_v<Char, char32_t>)
        {
            auto utf32 = QStringView(&c, 1).toUcs4();
            return Base::format(char32_t(utf32[0]), context);
        }
        else
        {
            static_assert(!std::is_same_v<Char, Char>, "Unsupported character type for QChar");
        }
	}
};

#ifdef __cpp_char8_t
template<>
struct fmt::formatter<QChar, char8_t> : fmt::formatter<std::u8string_view, char8_t>
{
	using Base = fmt::formatter<std::u8string_view, char8_t>;
	using Base::parse;
    
	template <typename FormatContext>
	auto format(QChar c, FormatContext& context) const
	{
        auto utf8 = QStringView(&c, 1).toUtf8();
        auto data = reinterpret_cast<const char8_t*>(utf8.data());
        auto size = std::size_t(utf8.size());
        return Base::format({data, size}, context);
	}
};
#endif
