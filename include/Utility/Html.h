#pragma once
#include <unordered_map>
#include <array>
#include <cassert>
#include <fmt/format.h>

#ifndef UTILITY_HTML_NO_QT
#  include <QVariant>
#  include <QJsonValue>
#  if !defined(UTILITY_HTML_NO_QT_XML) && __has_include(<QXmlStreamReader>) && __has_include(<QXmlStreamWriter>)
#    include <QXmlStreamReader>
#    include <QXmlStreamWriter>
#    define UTILITY_HTML_HAS_QT_XML 1
#  else
#    define UTILITY_HTML_HAS_QT_XML 0
#  endif
#endif

// something recommended to learn about before going on:
//   std::void_t
//     https://stackoverflow.com/questions/27687389/how-do-we-use-void-t-for-sfinae
//   cpo
//     https://zh.cppreference.com/w/cpp/ranges/cpo
//   CRTP
//     https://zh.cppreference.com/w/cpp/language/crtp

namespace Htmls
{
using namespace std::literals;

// forward declaration
template<typename T> std::string str(T&& element);
template<typename...> struct AlwaysFalse : std::false_type {};

// has member str
template<typename T, typename = void>
struct HasStrTrait : std::false_type {};

template<typename T>
struct HasStrTrait<T, std::void_t<
	decltype(std::declval<T>().str())>> : std::true_type {};

template<typename T>
constexpr bool hasStr = HasStrTrait<std::decay_t<T>>::value;

// T is attribute when T has a member 'name' and a member 'value'
template<typename T, typename = void>
struct IsAttributeTrait : std::false_type {};

template<typename T>
struct IsAttributeTrait<T, std::void_t<
	decltype(std::declval<T>().name()),
	decltype(std::declval<T>().value())>> : std::true_type {};

template<typename T>
constexpr bool isAttribute = IsAttributeTrait<std::decay_t<T>>::value;

// as str
template<typename T> struct AsStr;

template<typename T>
std::string asStr(T&& str) { return AsStr<std::decay_t<T>>{}.to(std::forward<T>(str)); }

// is string
template<typename T, typename = void>
struct IsString : std::false_type {};

template<typename T> // HINT: sizeof operator rejects incomplete types
struct IsString<T, std::void_t<decltype(sizeof(AsStr<std::decay_t<T>>))>> : std::true_type {};

template<typename T>
constexpr bool isString = IsString<T>::value;

// specifications of AsStr for types that used frequently
template<>
struct AsStr<std::nullptr_t> { static std::string to(std::nullptr_t) = delete; };

template<>
struct AsStr<std::string> { static std::string to(std::string str) { return str; } };

template<>
struct AsStr<std::string_view> { static std::string to(std::string_view str) { return std::string{str}; } };

template<>
struct AsStr<const char*> { static std::string to(const char* str) { return str; } };

#ifndef UTILITY_HTML_NO_QT
template<>
struct AsStr<QString> { static std::string to(const QString& qs) { return qs.toStdString(); } };

template<>
struct AsStr<QVariant> { static std::string to(const QVariant& v) { return v.toString().toStdString(); } };

template<>
struct AsStr<QJsonValue> { static std::string to(const QJsonValue& v) { return v.toVariant().toString().toStdString(); } };

template<>
struct AsStr<QJsonValueRef> { static std::string to(QJsonValueRef v) { return v.toVariant().toString().toStdString(); } };

template<>
struct AsStr<QJsonValueConstRef> { static std::string to(QJsonValueConstRef v) { return v.toVariant().toString().toStdString(); } };
#endif

// is element
template<typename T, typename = void>
struct IsElementTrait : std::false_type {};

template<typename T>
struct IsElementTrait<T, std::void_t<typename T::IsElement>> : std::true_type {};

template<typename T>
constexpr bool isElement = IsElementTrait<std::decay_t<T>>::value;

// wrap std::string for fmt::formatter
struct AttributeValue
{
	template<typename T>
	AttributeValue(T&& v) : value(std::forward<T>(v)) {}

	std::string value;
};

// operator""_attr
struct SimpleAttribute final : std::pair<std::string, std::string>
{
	using pair::pair;
	const std::string& name() const { return this->first; }
	const std::string& value() const { return this->second; }
};

template<typename T>
inline auto attr(std::string name, T&& value)
{
	return SimpleAttribute(std::move(name), fmt::format(FMT_STRING("{}"), value));
}

struct AttributeProxy final
{
	std::string_view name;

	template<typename T>
	auto operator=(T&& value) const
	{
		return attr(std::string(name), std::forward<T>(value));
	}
};

inline constexpr auto operator""_attr(const char* str, std::size_t size)
{
	return AttributeProxy{{str, size}};
}

// function style cpo
template<typename E>
struct From
{
	template<typename... Args>
	auto operator()(Args&&... args) const
	{
		E element;
		element.from(std::forward<Args>(args)...);
		return element;
	}
};

template<typename E>
struct FunctionStyle
{
	constexpr FunctionStyle() = default;

	template<typename... Subs>
	E operator()(Subs&&... subs) const
	{
		E element;
		element(std::forward<Subs>(subs)...);
		return element;
	}

	static constexpr auto from = From<E>{};
};

// element base class (CRTP)
template<typename T>
struct Element
{
	using ElementBase = Element;
	struct IsElement {};

	explicit Element(std::string_view tag) : tag(tag), attributes() {}
	Element(const Element& other) : tag(other.tag), attributes(other.attributes) {}
	Element(Element&& other) noexcept : tag(other.tag), attributes(std::move(other.attributes)) {}

	Element& operator=(const Element& other) &
	{
		assert(tag == other.tag);
		attributes = other.attributes;
	}

	Element& operator=(Element&& other) & noexcept
	{
		assert(tag == other.tag);
		attributes = std::move(other.attributes);
	}

	auto content() const = delete;

	template<typename Ti, typename... Ts>
	T& operator()(Ti&& ti, Ts&&... ts)
	{
		auto& self = static_cast<T&>(*this);
		return self[std::forward<Ti>(ti)](std::forward<Ts>(ts)...);
	}

	template<typename Ti, typename... Ts>
	T operator()(Ti&& ti, Ts&&... ts) const
	{
		T self = static_cast<const T&>(*this);
		return self[std::forward<Ti>(ti)](std::forward<Ts>(ts)...);
	}

	T& operator()() { return static_cast<T&>(*this); }
	T operator()() const { return static_cast<const T&>(*this); }

	template<typename A, std::enable_if_t<isAttribute<A>, int> = 0>
	T& operator[](A&& attr)
	{
		[[maybe_unused]] auto [pos, b] = this->attributes.try_emplace(
			std::string(std::forward<A>(attr).name()),
			std::forward<A>(attr).value()
		);
		assert(b);
		return static_cast<T&>(*this);
	}

	T& operator[](const AttributeProxy&)
	{
		static_assert(AlwaysFalse<T>{}, "attribute literal hasn't been assigned!");
	}

	std::string_view tag;
	std::unordered_map<std::string, AttributeValue> attributes;
};

template<typename T>
struct EmptyElement : Element<T>
{
	using Element<T>::Element;

	std::string str() const
	{
		if(this->attributes.empty())
			return fmt::format(FMT_STRING("<{} />"), this->tag);
		else
			return fmt::format(FMT_STRING("<{} {} />"), this->tag, fmt::join(this->attributes, " "));
	}
};

template<typename E>
struct Content
{
	using ContentBase = Content;
	Content(std::string content = {}) : contentValue(std::move(content)) {}

	std::string_view content() const& { return contentValue; }
	std::string&& content() && { return std::move(contentValue); }

	template<typename T, std::enable_if_t<isElement<T> || isString<T>, int> = 0>
	E& operator[](T&& element)
	{
		static_assert(isElement<T> || isString<T>);
		this->contentValue.append(str(std::forward<T>(element)));
		return static_cast<E&>(*this);
	}

	std::string contentValue;
};

template<typename E>
struct Container
{
	using ContainerBase = Container;

	template<typename T, std::enable_if_t<isElement<T> || isString<T>, int> = 0>
	E& operator[](T&& element)
	{
		this->items.emplace_back(str(std::forward<T>(element)));
		return static_cast<E&>(*this);
	}

	auto content() const { return fmt::join(this->items, ""); }

	std::vector<std::string> items;
};

// convert an element to string explicitly
template<typename T>
std::string str(T&& element)
{
	if constexpr (hasStr<T>)
		return std::forward<T>(element).str();
	else if constexpr (isString<T>)
		return asStr(std::forward<T>(element));
	else if constexpr (isElement<T>)
	{
		if(element.attributes.empty())
			return fmt::format(
				FMT_STRING("<{0}>{1}</{0}>"),
				element.tag,
				std::forward<T>(element).content());
		else
			return fmt::format(
				FMT_STRING("<{0} {1}>{2}</{0}>"),
				element.tag,
				fmt::join(element.attributes, " "),
				std::forward<T>(element).content());
	}
	else
		static_assert(AlwaysFalse<T>{}, "T is not a valid type to become html string!");
}
} // namespace Html

// format 'std::unordered_map<string, Html::AttributeValue>' with 'fmt::join'
template<>
struct fmt::formatter<std::pair<const std::string, Htmls::AttributeValue>>
{
	template<typename Context>
	constexpr auto parse(Context& context) const { return context.begin(); }

	template<typename Context>
	auto format(const std::pair<const std::string, Htmls::AttributeValue>& pair, Context& context) const
	{
		auto& [k, v] = pair;
		return fmt::format_to(context.out(), FMT_STRING(R"({}="{}")"), k, v.value);
	}
};

namespace Htmls
{
/// <br>
struct LineBreak final : EmptyElement<LineBreak>
{
	LineBreak() : EmptyElement("br"sv) {}
};

inline constexpr FunctionStyle<LineBreak> br;

/// <hr>
struct HorizontalRule final : EmptyElement<HorizontalRule>
{
	HorizontalRule() : EmptyElement("hr"sv) {}
};

inline constexpr FunctionStyle<HorizontalRule> hr;

/// <img>
struct Image final : EmptyElement<Image>
{
	Image() : EmptyElement("img"sv) {}

	template<typename Src, typename Alt>
	auto& from(Src&& source, Alt&& alternate)
	{
		return EmptyElement::operator()(
			"src"_attr=Htmls::str(std::forward<Src>(source)),
			"alt"_attr=Htmls::str(std::forward<Alt>(alternate))
		);
	}
};

inline constexpr FunctionStyle<Image> img;

/// <h1> ~ <h6>
template<int l>
struct Heading final : Element<Heading<l>>, Content<Heading<l>>
{
	static_assert((1 <= l) && (l <= 6));
	static constexpr int level = l;
	static constexpr std::array<char, 2> tagH{'h', char('1' + level - 1)};

	Heading(std::string content = {}) :
		Element<Heading<level>>({tagH.data(), tagH.size()}),
		Content<Heading<level>>(std::move(content))
	{}

	using Element<Heading>::operator();
	using Element<Heading>::operator[];
	using Content<Heading>::content;
	using Content<Heading>::operator[];
};

template<int level>
inline constexpr auto h = [](auto&& content) {
	auto heading = Heading<level>{};
	heading(std::forward<decltype(content)>(content));
	return heading;
};

inline auto operator""_h1(const char* str, std::size_t size) { return h<1>(std::string_view(str, size)); }
inline auto operator""_h2(const char* str, std::size_t size) { return h<2>(std::string_view(str, size)); }
inline auto operator""_h3(const char* str, std::size_t size) { return h<3>(std::string_view(str, size)); }
inline auto operator""_h4(const char* str, std::size_t size) { return h<4>(std::string_view(str, size)); }
inline auto operator""_h5(const char* str, std::size_t size) { return h<5>(std::string_view(str, size)); }
inline auto operator""_h6(const char* str, std::size_t size) { return h<6>(std::string_view(str, size)); }
inline constexpr auto h1 = FunctionStyle<Heading<1>>{};
inline constexpr auto h2 = FunctionStyle<Heading<2>>{};
inline constexpr auto h3 = FunctionStyle<Heading<3>>{};
inline constexpr auto h4 = FunctionStyle<Heading<4>>{};
inline constexpr auto h5 = FunctionStyle<Heading<5>>{};
inline constexpr auto h6 = FunctionStyle<Heading<6>>{};

/// <p>
struct Paragraph final : Element<Paragraph>, Content<Paragraph>
{
	Paragraph(std::string content = {}) : ElementBase("p"sv), ContentBase(std::move(content)) {}
	using ElementBase::operator();
	using ElementBase::operator[];
	using Content::operator[];
	using Content::content;
};

inline auto operator""_p(const char* str, std::size_t size) { return Paragraph({str, size}); }
inline constexpr FunctionStyle<Paragraph> p;

/// <pre>
struct Preformatted final : Element<Preformatted>, Content<Preformatted>
{
	Preformatted(std::string content = {}) : ElementBase("pre"sv), ContentBase(std::move(content)) {}
	using ElementBase::operator();
	using ElementBase::operator[];
	using Content::operator[];
	using Content::content;
};

inline auto operator""_pre(const char* str, std::size_t size) { return Preformatted({str, size}); }
inline constexpr FunctionStyle<Preformatted> pre;

/// <div>
struct Div final : Element<Div>, Container<Div>
{
	Div() : ElementBase("div"sv), ContainerBase() {}
	using ElementBase::operator();
	using ElementBase::operator[];
	using ContainerBase::content;
	using ContainerBase::operator[];
};

inline constexpr FunctionStyle<Div> div;

/// <table> <tr> <th> <td>
struct TableHeader final : Element<TableHeader>, Content<TableHeader>
{
	TableHeader(std::string content = {}) : ElementBase("th"sv), ContentBase(std::move(content)) {}
	using ElementBase::operator();
	using ElementBase::operator[];
	using Content::operator[];
	using Content::content;
};

inline auto operator""_th(const char* str, std::size_t size) { return TableHeader({str, size}); }
inline constexpr FunctionStyle<TableHeader> th;

struct TableData final : Element<TableData>, Content<TableData>
{
	TableData(std::string content = {}) : ElementBase("td"sv), ContentBase(std::move(content)) {}
	using ElementBase::operator();
	using ElementBase::operator[];
	using Content::operator[];
	using Content::content;
};

inline auto operator""_td(const char* str, std::size_t size) { return TableData({str, size}); }
inline constexpr FunctionStyle<TableData> td;

struct TableRow final : Element<TableRow>, Container<TableRow>
{
	TableRow() : ElementBase("tr"sv), ContainerBase() {}
	using ElementBase::operator();
	using ElementBase::operator[];
	using ContainerBase::operator[];
	using ContainerBase::content;

	template<typename Cast, typename Range>
	auto& from(const Cast& cast, Range&& row)
	{
		for(auto&& element : row) (*this)(cast(element));
		return *this;
	}

	template<typename Range>
	auto& from(Range&& range)
	{
		return from(td, std::forward<Range>(range));
	}
};

inline constexpr FunctionStyle<TableRow> tr;

struct Table final : Element<Table>, Container<Table>
{
	Table() : ElementBase("table"sv), ContainerBase() {}
	using ElementBase::operator();
	using ElementBase::operator[];
	using ContainerBase::operator[];
	using ContainerBase::content;

	template<typename Cast, typename Range>
	auto& from(const Cast& cast, Range&& rows)
	{
		for(auto&& row : rows) (*this)(cast(row));
		return *this;
	}

	template<typename Range>
	auto& from(Range&& range)
	{
		return from(tr.from, std::forward<Range>(range));
	}
};

inline constexpr FunctionStyle<Table> table;

/// <html>
struct Html final : Element<Html>, Container<Html>
{
	Html() : ElementBase("html"sv), ContainerBase() {}
	using ElementBase::operator();
	using ElementBase::operator[];
	using ContainerBase::operator[];
	using ContainerBase::content;
};

inline constexpr FunctionStyle<Html> html;

/// <head>
struct Head final : Element<Head>, Container<Head>
{
	Head() : ElementBase("head"sv), ContainerBase() {}
	using ElementBase::operator();
	using ElementBase::operator[];
	using ContainerBase::operator[];
	using ContainerBase::content;
};

inline constexpr FunctionStyle<Head> head;

/// <body>
struct Body final : Element<Body>, Container<Body>
{
	Body() : ElementBase("body"sv), ContainerBase() {}
	using ElementBase::operator();
	using ElementBase::operator[];
	using ContainerBase::operator[];
	using ContainerBase::content;
};

inline constexpr FunctionStyle<Body> body;

/// <meta>
struct Meta final : EmptyElement<Meta>
{
	Meta() : EmptyElement("meta"sv) {}
};

inline constexpr FunctionStyle<Meta> meta;

/// attr: style
/// TODO: wrap CSS
struct Style final
{
	static std::string_view name() { return "style"sv; }
	std::string_view value() const& { return styleValue; }
	std::string&& value() && { return std::move(styleValue); }
	std::string styleValue;
};

inline auto operator""_style(const char* str, std::size_t size) { return Style{{str, size}}; }

/// attr: class
struct Class final
{
	static std::string_view name() { return "class"sv; }
	std::string_view value() const& { return className; }
	std::string&& value() && { return std::move(className); }
	std::string className;
};

inline auto operator""_class(const char* str, std::size_t size) { return Class{{str, size}}; }

/// attr: span
struct Span final
{
	static std::string_view name() { return "span"sv; }
	std::string_view value() const& { return spanName; }
	std::string&& value() && { return std::move(spanName); }
	std::string spanName;
};

inline auto operator""_span(const char* str, std::size_t size) { return Span{{str, size}}; }

#if UTILITY_HTML_HAS_QT_XML
inline QByteArray formatDocument(std::string_view shtml)
{
	// TODO: check if shtml is valid XML
	QByteArray fhtml;
	QXmlStreamReader reader(QByteArray::fromRawData(shtml.data(), shtml.size()));
	QXmlStreamWriter writer(&fhtml);
	writer.setAutoFormatting(true);
	while(!reader.atEnd())
		if(reader.readNext(); !reader.isWhitespace()) // TODO: check if white spaces in <pre> kept
			writer.writeCurrentToken(reader);
	return fhtml;
}

template<typename E, std::enable_if_t<isElement<E>, int> = 0>
inline QByteArray formatDocument(const E& element)
{
	std::string shtml = str(element);
	return formatDocument(std::string_view(shtml));
}
#endif
} // namespace Html
