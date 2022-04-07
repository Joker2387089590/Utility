#pragma once
#include <chrono>
#include "RandomNumber.h"

// π
#if __cplusplus < 202002L
namespace Numbers
{
inline constexpr double pi = 3.141592653589793;
}
#else
#include <numbers>
namespace Numbers
{
using namespace std::numbers;
}
#endif

namespace SignalSequence
{
using namespace std::chrono_literals;
using Seconds				 = std::chrono::duration<double>;
inline constexpr auto oneSec = Seconds(1.);

template<typename T>
auto toSec(T t)
{
	return std::chrono::duration_cast<Seconds>(t);
}

inline namespace Operators
{
template<typename F, typename = void>
struct IsCallable : std::false_type {};

template<typename F>
struct IsCallable<F,
				  std::void_t<decltype(std::declval<F>()(Seconds{})),
							  std::enable_if<!std::is_integral_v<F>>>> :
	std::true_type
{};

template<typename F1, typename F2,
		 std::enable_if_t<IsCallable<F1>::value && IsCallable<F2>::value, int> = 0>
auto operator+(F1 f1, F2 f2)
{
	return [f1, f2](auto t) { return f1(t) + f2(t); };
}

template<typename F1, typename F2,
		 std::enable_if_t<IsCallable<F1>::value && IsCallable<F2>::value, int> = 0>
auto operator-(F1 f1, F2 f2)
{
	return [f1, f2](auto t) { return f1(t) - f2(t); };
}

template<typename F1, typename F2,
		 std::enable_if_t<IsCallable<F1>::value && IsCallable<F2>::value, int> = 0>
auto operator*(F1 f1, F2 f2)
{
	return [f1, f2](auto t) { return f1(t) * f2(t); };
}

template<typename F1, typename F2,
		 std::enable_if_t<IsCallable<F1>::value && IsCallable<F2>::value, int> = 0>
auto operator/(F1 f1, F2 f2)
{
	return [f1, f2](auto t) { return f1(t) / f2(t); };
}

// 没有和常数的直接加减，用 constant 来调用

template<typename F, std::enable_if_t<IsCallable<F>::value, int> = 0>
auto operator*(F f, double c)
{
	return [f, c](auto t) { return f(t) * c; };
}

template<typename F, std::enable_if_t<IsCallable<F>::value, int> = 0>
auto operator/(F f, double c)
{
	return [f, c](auto t) { return f(t) / c; };
}

template<typename F, std::enable_if_t<IsCallable<F>::value, int> = 0>
auto operator*(double c, F f)
{
	return [f, c](auto t) { return c * f(t); };
}

template<typename F, std::enable_if_t<IsCallable<F>::value, int> = 0>
auto operator/(double c, F f)
{
	return [f, c](auto t) { return c / f(t); };
}
} // namespace Operators

struct TimeRange
{
	template<typename Tb = Seconds, typename Te = Seconds>
	TimeRange(Tb begin, Te end) :
		begin(toSec(begin)),
		end(toSec(end))
	{
		if (this->begin > this->end) throw std::exception();
	}

	template<typename Te = Seconds>
	TimeRange(Te end) : TimeRange(Seconds{}, end) {}

	double sec() const { return (end - begin).count(); }
	double ms() const { return sec() * 1e3; }

	template<typename V>
	auto generate(double fs) const
	{
		const auto count = std::size_t(fs * (end - begin).count());
		const double dT	 = 1 / fs;
		V vec;
		vec.reserve(count);
		for (std::size_t i = 0; i != count; ++i) vec.push_back(begin.count() + i * dT);
		return vec;
	}

public:
	Seconds begin;
	Seconds end;
};

template<typename T = Seconds>
auto sin(double f, T phase = {})
{
	return [=](auto t) {
		auto seconds = (toSec(t) + phase).count();
		return std::sin(2 * Numbers::pi * f * seconds);
	};
}

template<typename T = Seconds>
auto cos(double f, T phase = {})
{
	return [=](auto t) {
		auto seconds = (toSec(t) + phase).count();
		return std::cos(2 * Numbers::pi * f * seconds);
	};
}

auto constant(double value)
{
	return [=](auto) { return value; };
}

template<typename T = Seconds>
auto delta(double value, T delay)
{
	return [=](auto t) { return t == delay ? value : 0.; };
}

auto randomNoise()
{
	return [](auto) {
		static RandomNumber machine(-1., 1.);
		return machine();
	};
}

auto gaussNoise(double value, double stddev = 1.0)
{
	return [=](auto) {
		static GaussRandom machine(value, stddev);
		return machine();
	};
}

template<template<typename...> typename Vec, typename G>
auto generate(TimeRange r, double fs, G g)
{
	const auto count = std::size_t(std::floor(r.sec() * fs));
	const double dT	 = 1 / fs;
	Vec<double> vec;
	vec.reserve(count);
	for (std::size_t i = 0; i != count; ++i) vec.push_back(g(r.begin + Seconds(i * dT)));
	return vec;
}
} // namespace SignalSequence
