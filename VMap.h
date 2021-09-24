#pragma once
#include <QMap>

namespace _vmap_Detail
{
	template<typename T>
	struct Trait : std::false_type {};

	template<typename K, typename V>
	struct Trait<QMap<K, V>> : std::true_type {};

	template<typename Q>
	constexpr bool isQMap = Trait<std::decay_t<Q>>::value;
}

template<typename Q, std::enable_if_t<_vmap_Detail::isQMap<Q>, int> = 0>
class VMap
{
public:
	template<typename Qm>
	VMap(Qm&& qmap) : m(std::forward<Qm>(qmap)) {}

	auto begin()	{ return m.keyValueBegin(); }
	auto end()		{ return m.keyValueEnd(); }
	auto cbegin()	{ return std::as_const(m).keyValueBegin(); }
	auto cend()		{ return std::as_const(m).keyValueEnd(); }

private:
	using Type = std::conditional_t<std::is_rvalue_reference_v<Q>,
		std::remove_reference_t<Q>, Q>;
	Type m;
};

template<typename Q> VMap(Q&&) -> VMap<Q>;
