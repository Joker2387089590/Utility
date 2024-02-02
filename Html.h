#pragma once
#include <unordered_map>
#include <array>
#include <cassert>
#include <fmt/format.h>
#include "LazyGenerator.h"

namespace Htmls
{
using namespace std::literals;

// has str
template<typename T, typename = void>
struct HasStrTrait : std::false_type {};
template<typename T>
struct HasStrTrait<T, std::void_t<
	decltype(std::declval<T>().str())>> : std::true_type {};
template<typename T>
constexpr bool hasStr = HasStrTrait<std::decay_t<T>>::value;

// is attribute
template<typename T, typename = void>
struct IsAttributeTrait : std::false_type {};
template<typename T>
struct IsAttributeTrait<T, std::void_t<
	decltype(std::declval<T>().name()),
	decltype(std::declval<T>().value())>> : std::true_type {};
template<typename T>
constexpr bool isAttribute = IsAttributeTrait<std::decay_t<T>>::value;

// is string
struct AsStr {} inline constexpr asStr;

template<typename T, typename = void>
struct IsString : std::false_type {};
template<typename T>
struct IsString<T, std::void_t<decltype(asStr | std::declval<T>())>> : std::true_type {};
template<typename T>
constexpr bool isString = IsString<T>::value;

inline std::string operator|(AsStr, std::string str) { return str; }
inline std::string operator|(AsStr, std::string_view str) { return std::string(str); }
inline std::string operator|(AsStr, const char* str) { return std::string(str); }
void operator|(AsStr, std::nullptr_t) = delete;

// is element
template<typename T, typename = void>
struct IsElementTrait : std::false_type {};
template<typename T>
struct IsElementTrait<T, std::void_t<typename T::IsElement>> : std::true_type {};
template<typename T>
constexpr bool isElement = IsElementTrait<std::decay_t<T>>::value;

// wrap std::string for fmt::formatter
struct AttributeValue { std::string value; };

template<typename T> std::string str(T&& element);

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
};

template<typename T>
struct Element
{
	using ElementBase = Element;
	struct IsElement {};

	explicit Element(std::string_view tag) : tag(tag), attributes() {}

	auto content() const = delete;

	template<typename Ti, typename... Ts>
	T& operator()(Ti&& ti, Ts&&... ts)
	{
		auto& self = static_cast<T&>(*this);
		return self(std::forward<Ti>(ti))(std::forward<Ts>(ts)...);
	}

	T& operator()() { return *static_cast<T*>(this); }

	template<typename A, std::enable_if_t<isAttribute<A>, int> = 0>
	T& operator()(A&& attr)
	{
		auto [pos, b] = this->attributes.try_emplace(
			std::string(std::forward<A>(attr).name()),
			Lazys::lazy<AttributeValue>(std::forward<A>(attr).value())
		);
		assert(b);
		return static_cast<T&>(*this);
	}

	const std::string_view tag;
	std::unordered_map<std::string, AttributeValue> attributes;
};

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

struct AttributeProxy
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

	template<typename T, std::enable_if_t<(isElement<T> || isString<T>), int> = 0>
	E& operator()(T&& element)
	{
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
	E& operator()(T&& element)
	{
		this->items.emplace_back(str(std::forward<T>(element)));
		return static_cast<E&>(*this);
	}

	auto content() const { return fmt::join(this->items, ""); }

	std::vector<std::string> items;
};

template<typename...> struct AlwaysFalse : std::false_type {};

// convert an element to string explicitly
template<typename T>
std::string str(T&& element)
{
	if constexpr (hasStr<T>)
		return std::forward<T>(element).str();
	else if constexpr (isString<T>)
	{
		// overload operator for name-lookup
		// which is 'the dark sides of the inner workings of C++'
		// put your overloads into global namespace when this line cannot be compiled.
		return asStr | std::forward<T>(element);
	}
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
};

template<typename Src, typename Alt>
auto img(Src&& source, Alt&& alternate)
{
	Image img;
	img("src"_attr=str(std::forward<Src>(source)),
		"alt"_attr=str(std::forward<Alt>(alternate)));
	return img;
}

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
	using Content<Heading>::content;
	using Content<Heading>::operator();
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
inline constexpr auto h1 = h<1>;
inline constexpr auto h2 = h<2>;
inline constexpr auto h3 = h<3>;
inline constexpr auto h4 = h<4>;
inline constexpr auto h5 = h<5>;
inline constexpr auto h6 = h<6>;

/// <p>
struct Paragraph final : Element<Paragraph>, Content<Paragraph>
{
	Paragraph(std::string content = {}) : ElementBase("p"sv), ContentBase(std::move(content)) {}
	using ElementBase::operator();
	using Content::operator();
	using Content::content;
};

inline auto operator""_p(const char* str, std::size_t size) { return Paragraph({str, size}); }
inline constexpr FunctionStyle<Paragraph> p;

/// <pre>
struct Preformatted final : Element<Preformatted>, Content<Preformatted>
{
	Preformatted(std::string content = {}) : ElementBase("pre"sv), ContentBase(std::move(content)) {}
	using ElementBase::operator();
	using Content::operator();
	using Content::content;
};

inline auto operator""_pre(const char* str, std::size_t size) { return Preformatted({str, size}); }
inline constexpr FunctionStyle<Preformatted> pre;

/// <div>
struct Div final : Element<Div>, Container<Div>
{
	Div() : ElementBase("div"sv), ContainerBase() {}
	using ElementBase::operator();
	using ContainerBase::operator();
	using ContainerBase::content;
};

inline constexpr FunctionStyle<Div> div;

/// <table> <tr> <th> <td>
struct TableHeader final : Element<TableHeader>, Content<TableHeader>
{
	TableHeader(std::string content = {}) : ElementBase("th"sv), ContentBase(std::move(content)) {}
	using ElementBase::operator();
	using Content::operator();
	using Content::content;
};

inline auto operator""_th(const char* str, std::size_t size) { return TableHeader({str, size}); }
inline constexpr FunctionStyle<TableHeader> th;

struct TableData final : Element<TableData>, Content<TableData>
{
	TableData(std::string content = {}) : ElementBase("td"sv), ContentBase(std::move(content)) {}
	using ElementBase::operator();
	using Content::operator();
	using Content::content;
};

inline auto operator""_td(const char* str, std::size_t size) { return TableData({str, size}); }
inline constexpr FunctionStyle<TableData> td;

struct TableRow final : Element<TableRow>, Container<TableRow>
{
	TableRow() : ElementBase("tr"sv), ContainerBase() {}
	using ElementBase::operator();
	using ContainerBase::operator();
	using ContainerBase::content;

	template<typename Range>
	auto& from(Range&& row)
	{
		for(auto&& element : row) (*this)(element);
		return *this;
	}

	template<typename Cast, typename Range>
	auto& from(Cast&& cast, Range&& row)
	{
		for(auto&& element : row) (*this)(cast(element));
		return *this;
	}
};

struct TableRowFunc : FunctionStyle<TableRow>
{
	constexpr TableRowFunc() = default;
	using FunctionStyle::operator();

	template<typename Range>
	static auto from(Range&& row)
	{
		TableRow tr;
		tr.from(std::forward<Range>(row));
		return tr;
	}

	template<typename Cast, typename Range>
	static auto from(Cast&& cast, Range&& row)
	{
		TableRow tr;
		tr.from(std::forward<Cast>(cast), std::forward<Range>(row));
		return tr;
	}
};

inline constexpr TableRowFunc tr;

struct Table final : Element<Table>, Container<Table>
{
	Table() : ElementBase("table"sv), ContainerBase() {}
	using ElementBase::operator();
	using ContainerBase::operator();
	using ContainerBase::content;

	template<typename Range>
	auto& from(Range&& rows)
	{
		for(auto&& row : rows) (*this)(tr.from(row));
		return *this;
	}
};

struct TableFunc : FunctionStyle<Table>
{
	template<typename Range>
	static auto from(Range&& rows)
	{
		Table table;
		table.from(tr.from(rows));
		return table;
	}
};

inline constexpr FunctionStyle<Table> table;

/// <html>
struct Html final : Element<Html>, Container<Html>
{
	Html() : ElementBase("html"sv), ContainerBase() {}
	using ElementBase::operator();
	using ContainerBase::operator();
	using ContainerBase::content;
};

inline constexpr FunctionStyle<Html> html;

/// <head>
struct Head final : Element<Head>, Container<Head>
{
	Head() : ElementBase("head"sv), ContainerBase() {}
	using ElementBase::operator();
	using ContainerBase::operator();
	using ContainerBase::content;
};

inline constexpr FunctionStyle<Head> head;

/// <body>
struct Body final : Element<Body>, Container<Body>
{
	Body() : ElementBase("body"sv), ContainerBase() {}
	using ElementBase::operator();
	using ContainerBase::operator();
	using ContainerBase::content;
};

inline constexpr FunctionStyle<Body> body;

struct Document
{
	std::string declaration{"<!DOCTYPE html>"s};


};

/// attr: style
struct Style final
{
	static std::string_view name() { return "style"sv; }
	std::string_view value() const& { return styleValue; }
	std::string value() && { return std::move(styleValue); }
	std::string styleValue;
};

inline auto operator""_style(const char* str, std::size_t size) { return Style{{str, size}}; }
inline constexpr FunctionStyle<Style> style;
} // namespace Html
