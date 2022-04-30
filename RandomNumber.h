#pragma once
#include <random>

namespace Randoms
{
	template<typename D>
	class Generator
	{
	public:
		using type = typename D::result_type;

		template<typename... Args>
		Generator(Args&&... args) :
			generator(std::random_device{}()),
			distribution(std::forward<Args>(args)...)
		{}
		operator type() { return distribution(generator); }
		type operator()() { return distribution(generator); }

	private:
		std::default_random_engine generator;
		D distribution;
	};

	template<typename T>
	using UniformDistribution
		= std::conditional_t<std::is_integral_v<T>,
							 std::uniform_int_distribution<T>,
							 std::conditional_t<std::is_floating_point_v<T>,
												std::uniform_real_distribution<T>,
												void>>;

	/// 随机数生成器
	/// Usage:
	/// static RandomNumber randomMachine(0.0, 1.0); // 设为静态或全局变量
	/// double r1 = randomMachine;
	/// double r2 = randomMachine();
	/// ...
	template<typename T = int>
	class RandomNumber : public Generator<UniformDistribution<T>>
	{
		using Base = Generator<UniformDistribution<T>>;
	public:
		using type = typename Base::type;
		using Base::operator();
		using Base::operator type;

		RandomNumber(T l = std::numeric_limits<T>::min(),
					 T u = std::numeric_limits<T>::max()) :
			Base(l, u)
		{}
	};

	template<typename T1, typename T2>
	RandomNumber(T1, T2) -> RandomNumber<std::common_type_t<T1, T2>>;

	template<typename T = double,
			 std::enable_if_t<std::is_floating_point_v<T>, int> = 0>
	class GaussRandom : public Generator<std::normal_distribution<T>>
	{
		using Base = Generator<std::normal_distribution<T>>;
	public:
		using type = typename Base::type;
		using Base::operator();
		using Base::operator type;

		GaussRandom(T mean, T stddev = 1.0) : Base(mean, stddev) {}
		static auto range3Sigma(T lower, T upper)
		{
			return GaussRandom((lower + upper) / 2, std::abs(upper - lower) / 6);
		}
	};
}

using Randoms::RandomNumber;
using Randoms::GaussRandom;
