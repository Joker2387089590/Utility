#pragma once
#include <unordered_map>
#include <array>
#include <cassert>
#include <fmt/format.h>
#include "LazyGenerator.h"
#include "TypeList.h"

namespace Html
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
template<typename T>
constexpr bool isString =
	Types::TList<std::string, std::string_view, const char*>::anyIs<std::decay_t<T>>;

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
		return std::string(std::forward<T>(element));
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
struct fmt::formatter<std::pair<const std::string, Html::AttributeValue>>
{
	template<typename Context>
	constexpr auto parse(Context& context) const { return context.begin(); }

	template<typename Context>
	auto format(const std::pair<std::string, Html::AttributeValue>& pair, Context& context) const
	{
		auto& [k, v] = pair;
		return fmt::format_to(context.out(), FMT_STRING(R"({}="{}")"), k, v.value);
	}
};

namespace Html
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

	using Element<Heading<l>>::operator();
	using Content<Heading<l>>::content;
	using Content<Heading<l>>::operator();
};

inline auto operator""_h1(const char* str, std::size_t size) { return Heading<1>({str, size}); }
inline auto operator""_h2(const char* str, std::size_t size) { return Heading<2>({str, size}); }
inline auto operator""_h3(const char* str, std::size_t size) { return Heading<3>({str, size}); }
inline auto operator""_h4(const char* str, std::size_t size) { return Heading<4>({str, size}); }
inline auto operator""_h5(const char* str, std::size_t size) { return Heading<5>({str, size}); }
inline auto operator""_h6(const char* str, std::size_t size) { return Heading<6>({str, size}); }

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
struct TableRow {};
struct TableHeader {};
struct TableData {};
struct Table final : Element<Table>, Container<Table>
{

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
