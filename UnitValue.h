#pragma once
#include <cmath>
#include <array>
#include <QString>

namespace Units
{
inline constexpr char16_t unitSet[] = u"fpnμm\0kMG";
inline constexpr QStringView unitSetStr(unitSet, std::array{unitSet}.size() - 1);

struct UnitValue
{
public:

	template<typename T> constexpr UnitValue(T&& v) noexcept : value(v) {}
	operator QString()	const { return str(); }
	operator double()	const { return value; }

	int exp()       const { return (value == 0.) ? 0. : std::floor(std::log10(std::abs(value))); }
	int unitExp()   const { return std::min(std::max(exp(), -15), 9); }

    QChar unit()    const { return unitSet[int(std::floor(unitExp() / 3.)) + 5]; }
	double count()  const { return value * std::pow(10, std::ceil(-unitExp() / 3.) * 3); }

	QString str(char format = 'f', int precision = 3) const
	{
        const QChar units[2]{unit(), '\0'};
        return QString::number(count(), format, precision) + QString(units);
	}
	QString addUnit(QString u, char format = 'f', int precision = 3)
	{
		return this->str(format, precision) + u;
	}

public:
	double value;
};

inline constexpr auto operator*(const UnitValue& l, const UnitValue& r) noexcept { return UnitValue{l.value * r.value}; }
inline constexpr auto operator/(const UnitValue& l, const UnitValue& r) noexcept { return UnitValue{l.value / r.value}; }
inline constexpr auto operator+(const UnitValue& l, const UnitValue& r) noexcept { return UnitValue{l.value + r.value}; }
inline constexpr auto operator-(const UnitValue& l, const UnitValue& r) noexcept { return UnitValue{l.value - r.value}; }
template<typename T> inline constexpr auto operator*(const T& l, const UnitValue& r) noexcept { return UnitValue{l} * r; }
template<typename T> inline constexpr auto operator/(const T& l, const UnitValue& r) noexcept { return UnitValue{l} / r; }
template<typename T> inline constexpr auto operator+(const T& l, const UnitValue& r) noexcept { return UnitValue{l} + r; }
template<typename T> inline constexpr auto operator-(const T& l, const UnitValue& r) noexcept { return UnitValue{l} - r; }
template<typename T> inline constexpr auto operator*(const UnitValue& l, const T& r) noexcept { return UnitValue{r} * l; }
template<typename T> inline constexpr auto operator/(const UnitValue& l, const T& r) noexcept { return UnitValue{r} / l; }
template<typename T> inline constexpr auto operator+(const UnitValue& l, const T& r) noexcept { return UnitValue{r} + l; }
template<typename T> inline constexpr auto operator-(const UnitValue& l, const T& r) noexcept { return UnitValue{r} - l; }
}

using Units::UnitValue;
