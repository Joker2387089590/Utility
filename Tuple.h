#pragma once
#include <tuple>

template<std::size_t i, typename... Ts> struct AtHelper;

template<std::size_t i, typename T, typename... Ts>
struct AtHelper<i, T, Ts...> { using type = typename AtHelper<i - 1, Ts...>::type; };

template<typename T, typename... Ts>
struct AtHelper<0, T, Ts...> { using type = T; };

template<std::size_t i> struct AtHelper<i> {};

template<std::size_t i, typename T>
struct IndexedType
{
	using type = T;
	template<typename U> constexpr IndexedType(U&& d) : data(std::forward<U>(d)) {}
	constexpr IndexedType() : data() {}
	T data;
};

template<typename... Ts>
struct TypeList { template<std::size_t... is> class SizeList; };

template<typename... Ts>
template<std::size_t... is>
class TypeList<Ts...>::SizeList : private IndexedType<is, Ts>...
{
public:
	template<std::size_t i> using At = typename AtHelper<i, Ts...>::type;
private:
	template<std::size_t i> using Base = IndexedType<i, At<i>>;

public:
	SizeList() : Base<is>()... {}
	template<typename... Args> SizeList(Args&&... args) : Base<is>(args)... {}

	SizeList(SizeList&&) = default;
	SizeList& operator=(SizeList&&) = default;
	SizeList(const SizeList&) = default;
	SizeList& operator=(const SizeList&) = default;
	~SizeList() = default;

public:
	template<typename T>
	static constexpr std::size_t indexOfFirst()
	{
		std::size_t i = 0;
		auto test = [&](bool b) { if(!b) ++i; return b; };
		(test(std::is_same_v<At<is>, T>) || ...);
		return i;
	}

	template<std::size_t i>
	constexpr auto get() const { return self<i>()->data; }

	template<typename T>
	constexpr auto get() const { return get<indexOfFirst<T>()>(); }

	static constexpr std::size_t size() { return sizeof...(Ts); }

protected:
	template<std::size_t i> Base<i>* self() { return this; }
	template<std::size_t i> const Base<i>* self() const { return this; }

	template<typename F>
	auto forEach(F f) const { return forEachHelper(f, self<is>()...); }
	template<typename F>
	auto forEach(F f) { return forEachHelper(f, self<is>()...); }

	template<typename C>
	auto filter(C c) { return filterHelper(filterResult, c, self<is>()...); }
	template<typename C>
	auto filter(C c) const { return filterHelper(filterResult, c, self<is>()...); }

private:
	template<typename F, typename T, typename... Ti>
	auto forEachHelper(F f, T* t, Ti*... ts) const
	{
		return forEachHelper([f, t](auto... ends) { return f(t, ends...); }, ts...);
	}
	template<typename F>
	auto forEachHelper(F f) const { return f(); }

	template<typename F, typename T, typename... Ti>
	auto forEachHelper(F f, T* t, Ti*... ts)
	{
		return forEachHelper([f, t](auto... ends) { return f(t, ends...); }, ts...);
	}
	template<typename F>
	auto forEachHelper(F f) { return f(); }

private:
	inline static constexpr auto filterResult = [](auto... good) {
		return [good...](auto&& p) { return p(good...); };
	};

	template<typename F, typename C, typename T, typename... Ti>
	auto filterHelper(F f, C c, T* t, Ti*... ts)
	{
		using Condition = decltype(c(t));
		if constexpr (Condition::value)
			return filterHelper([f, t](auto... ends) { return f(t, ends...); }, c, ts...);
		else
			return filterHelper(f, c, ts...);
	}

	template<typename F, typename C>
	auto filterHelper(F f, C) { return f(); }

	template<typename F, typename C, typename T, typename... Ti>
	auto filterHelper(F f, C c, T* t, Ti*... ts) const
	{
		using Condition = decltype(c(t));
		if constexpr (Condition::value)
			return filterHelper([f, t](auto... ends) { return f(t, ends...); }, c, ts...);
		else
			return filterHelper(f, c, ts...);
	}

	template<typename F, typename C>
	auto filterHelper(F f, C) const { return f(); }
};

template<typename... Ts, std::size_t... i>
auto reduceTypeSize(std::index_sequence<i...>) ->
	typename TypeList<Ts...>::template SizeList<i...>;

template<typename... Ts>
using ReduceTypeSize =
	decltype(reduceTypeSize<Ts...>(std::make_index_sequence<sizeof...(Ts)>{}));

template<typename... Ts> auto makeTuple(Ts&&... ts);

template<typename... Ts>
class Tuple : public ReduceTypeSize<Ts...>
{
	using Base = ReduceTypeSize<Ts...>;
public:
	template<typename... Args>
	constexpr Tuple(Args&&... args) : Base(std::forward<Args>(args)...) {}

	constexpr Tuple() : Base() {}

	// TODO: Tuple(std::tuple<...>)
	// TODO: Tuple(std::pair<...>)

	using Base::At;
	using Base::get;
	using Base::indexOfFirst;
	using Base::size;

	template<typename> using True = std::true_type;

	template<typename... Us>
	Tuple<Ts..., Us...> append(const Tuple<Us...>& other) const
	{
		auto vThis = [](auto*... ts) {
			return [ts...](auto*... us) {
				return Tuple<Ts..., Us...>(ts->data..., us->data...);
			};
		};
		auto tThis = this->forEach(vThis);
		auto vOther = [&tThis](auto*... us) { return tThis(us...); };
		return other.forEach(vOther);
	}

	template<typename... T>
	auto remove() const
	{
		auto c = [](auto* t) {
			using Ti = typename std::decay_t<decltype(*t)>::type;
			constexpr bool isOneOf = std::disjunction_v<std::is_same<T, Ti>...>;
			return std::bool_constant<!isOneOf>{};
		};
		auto f = [](auto*... good) {
			return Tuple<typename std::decay_t<decltype(*good)>::type...>(good->data...);
		};
		return Base::filter(c)(f);
	}

	template<typename F>
	auto map(F f)
	{
		return Base::forEach([f](auto*... ts) { return makeTuple(f(ts->data)...); });
	}

	template<typename F>
	auto map(F f) const
	{
		return Base::forEach([f](auto*... ts) { return makeTuple(f(ts->data)...); });
	}

	auto tie() noexcept { return this->map([](auto& data) { return std::ref(data); }); }
	auto tie() const noexcept { return this->map([](auto& data) { return std::cref(data); }); }
};

template<typename... Ts> Tuple(Ts...) -> Tuple<Ts...>;
template<typename... Ts> Tuple(std::tuple<Ts...>) -> Tuple<Ts...>;
template<typename T1, typename T2> Tuple(std::pair<T1, T2>) -> Tuple<T1, T2>;

template <typename T> struct UnwrapRef { using type = T; };
template <typename T> struct UnwrapRef<std::reference_wrapper<T>> { using type = T&; };
template <typename T>
using UnwrapDecay = typename UnwrapRef<typename std::decay<T>::type>::type;

template<typename... Ts>
auto makeTuple(Ts&&... ts) { return Tuple<UnwrapDecay<Ts>...>(std::forward<Ts>(ts)...); }
