#pragma once
#include <QMap>
#include <QHash>

template<typename Qp>
class VMap
{
public:
	template<typename Qx> explicit VMap(Qx&& m) : m(&m) {}
	auto begin()  const { return m->keyValueBegin(); }
	auto end()    const { return m->keyValueEnd(); }
	auto cbegin() const { return std::as_const(*m).keyValueBegin(); }
	auto cend()   const { return std::as_const(*m).keyValueEnd(); }
private:
	Qp m;
};

template<typename T> VMap(T &&) -> VMap<std::remove_reference_t<T> *>;
template<typename T> VMap(T const&&) -> VMap<std::remove_reference_t<T> const*>;
