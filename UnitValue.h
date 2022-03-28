#pragma once
#include <cmath>
#include <QString>

struct UnitValue
{
public:
	template<typename T> constexpr UnitValue(T&& v) noexcept : value(v) {}
	operator QString()	const { return str(); }
	operator double()	const { return value; }

	int exp()       const { return (value == 0.) ? 0. : std::floor(std::log10(std::abs(value))); }
	int unitExp()   const { return std::min(std::max(exp(), -15), 9); }
	QChar unit()    const { return u"fpnμm\0kMG"[int(std::floor(unitExp() / 3.)) + 5]; }
	double count()  const { return value * std::pow(10, std::ceil(-unitExp() / 3.) * 3); }
	QString str(char format = 'f', int precision = 3) const
	{
		return QString::number(count(), format, precision) + unit();
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
