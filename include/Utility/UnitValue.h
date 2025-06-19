#pragma once
#include <cmath>
#include <string_view>
#include <QString>

namespace Units
{
using namespace std::literals;
inline constexpr auto unitSet{u"fpnμm\0kMG"sv};
inline constexpr QStringView unitSetStr(unitSet);
inline constexpr int unitExpValue[] = { -15, -12, -9, -6, -3, 0, 3, 6, 9 };
inline constexpr double unitPower[] = { 1e-15, 1e-12, 1e-9, 1e-6, 1e-3, 1, 1e3, 1e6, 1e9 };

inline constexpr double powerOf(QChar c)
{
	if(c == QChar(u'\0'))
		return 1;
	else
		return unitPower[unitSet.find(c.unicode())];
}

struct UnitValue
{
public:
	constexpr UnitValue(double v) noexcept : value(v) {}

	// "5000M"
	UnitValue(const QString& text) : value(std::numeric_limits<double>::quiet_NaN())
	{
		if(text.isEmpty()) return;

		QStringView numStr(text);
		double power = 1;
		QChar last = text.back();
		auto pos = unitSet.find(last.unicode());
		if(pos != unitSet.npos)
		{
			power = unitPower[pos];
			numStr.chop(1);
		}

		bool ok = false;
		double num = numStr.toDouble(&ok);
		if(ok) value = num * power;
	}

	int exp()      const { return (value == 0.) ? 0. : std::floor(std::log10(std::abs(value))); }
	int unitExp()  const { return std::clamp(exp(), -15, 9); }

	QChar unit()   const { return unitSet[unitExp() / 3 + 5]; }
	double count() const { return value * std::pow(10, std::ceil(-unitExp() / 3.) * 3); }

	QString str(char format = 'f', int precision = 3) const
	{
        const QChar units[2]{unit(), u'\0'};
        return QString::number(count(), format, precision) + QString(units);
	}

	QString addUnit(QString u, char format = 'f', int precision = 3)
	{
		return this->str(format, precision) + u;
	}

	operator QString()	const { return str(); }
	operator double()	const { return value; }

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
