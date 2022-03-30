#pragma once
#include <chrono>
#include <QtMath>
#include "RandomNumber.h"

namespace SignalSequence
{
	using namespace std::chrono_literals;
	using Seconds = std::chrono::duration<double>;

	template<typename T> auto toSec(T t) { return std::chrono::duration_cast<Seconds>(t); }

	inline namespace Operators
	{

		template<typename F, typename = void>
		struct IsCallable : std::false_type {};

		template<typename F>
		struct IsCallable<F, std::void_t<decltype(std::declval<F>()(Seconds{})),
										 std::enable_if<!std::is_integral_v<F>>>> : std::true_type {};
		template<typename F1, typename F2, std::enable_if_t<IsCallable<F1>::value && IsCallable<F2>::value, int> = 0>
		auto operator+(F1 f1, F2 f2) { return [f1, f2](auto t) { return f1(t) + f2(t); }; }

		template<typename F1, typename F2, std::enable_if_t<IsCallable<F1>::value && IsCallable<F2>::value, int> = 0>
		auto operator-(F1 f1, F2 f2) { return [f1, f2](auto t) { return f1(t) - f2(t); }; }

		template<typename F1, typename F2, std::enable_if_t<IsCallable<F1>::value && IsCallable<F2>::value, int> = 0>
		auto operator*(F1 f1, F2 f2) { return [f1, f2](auto t) { return f1(t) * f2(t); }; }

		template<typename F1, typename F2, std::enable_if_t<IsCallable<F1>::value && IsCallable<F2>::value, int> = 0>
		auto operator/(F1 f1, F2 f2) { return [f1, f2](auto t) { return f1(t) / f2(t); }; }

		template<typename F, std::enable_if_t<IsCallable<F>::value, int> = 0>
		auto operator*(F f, double c) { return [f, c](auto t) { return f(t) * c; }; }

		template<typename F, std::enable_if_t<IsCallable<F>::value, int> = 0>
		auto operator/(F f, double c) { return [f, c](auto t) { return f(t) / c; }; }

		template<typename F, std::enable_if_t<IsCallable<F>::value, int> = 0>
		auto operator*(double c, F f) { return [f, c](auto t) { return c * f(t); }; }

		template<typename F, std::enable_if_t<IsCallable<F>::value, int> = 0>
		auto operator/(double c, F f) { return [f, c](auto t) { return c / f(t); }; }
	}

	struct TimeRange
	{
		template<typename Tb = Seconds, typename Te = Seconds>
		TimeRange(Tb begin, Te end) : begin(toSec(begin)), end(toSec(end))
		{
			if(this->begin > this->end) throw std::exception();
		}

		template<typename Te = Seconds>
		TimeRange(Te end) : TimeRange(Seconds{}, end) {}

		double sec() const { return (end - begin).count(); }
		double ms() const { return sec() * 1e3; }

		struct Iter
		{
			double operator*() const { return curT; }
			Iter& operator++() { curT += dT; return *this; }
			bool operator==(const Iter&) const { return curT < end; }

			double curT;
			const double end;
			const double dT;
		};

		Iter span(double fs) const { return { begin.count(), end.count(), 1 / fs }; }

	public:
		Seconds begin;
		Seconds end;
	};

	auto begin(TimeRange::Iter iter) { return iter; }
	auto end(TimeRange::Iter iter) { return iter; }

	template<typename T = Seconds>
	auto sin(double f, T phase = {})
	{
		return [f, phase](auto t)
		{
			auto seconds = (toSec(t) + phase).count();
			return std::sin(2 * M_PI * f * seconds);
		};
	}

	template<typename T = Seconds>
	auto cos(double f, T phase = {})
	{
		return [f, phase](auto t)
		{
			auto seconds = (toSec(t) + phase).count();
			return std::cos(2 * M_PI * f * seconds);
		};
	}

	auto constant(double value)
	{
		return [value](auto) { return value; };
	}

	template<typename T = Seconds>
	auto delta(double value, T delay)
	{
		return [value, delay](auto t) { return t == delay ? value : 0.; };
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
		std::size_t i = 0;
		Vec<double> vec(std::size_t(std::floor(r.sec() * fs)));
		for(auto& data : vec) data = g(r.begin + Seconds(i++ / fs));
		return vec;
	}
}
