#pragma once
#include <QMap>
#include <QHash>

template<typename Q>
class VMap
{
public:
	template<typename Qx> explicit VMap(Qx&& m) : m(std::forward<Qx>(m)) {}
	auto begin()	{ return m.keyValueBegin(); }
	auto end()		{ return m.keyValueEnd(); }
	auto cbegin()	{ return std::as_const(m).keyValueBegin(); }
	auto cend()		{ return std::as_const(m).keyValueEnd(); }
private:
	Q&& m;
};

template<typename K, typename V> VMap(QMap<K, V>      &) -> VMap<QMap<K, V>&>;
template<typename K, typename V> VMap(QMap<K, V>     &&) -> VMap<QMap<K, V>&&>;
template<typename K, typename V> VMap(QMap<K, V> const&) -> VMap<QMap<K, V> const&>;

template<typename K, typename V> VMap(QHash<K, V>      &) -> VMap<QHash<K, V>&>;
template<typename K, typename V> VMap(QHash<K, V>     &&) -> VMap<QHash<K, V>&&>;
template<typename K, typename V> VMap(QHash<K, V> const&) -> VMap<QHash<K, V> const&>;
