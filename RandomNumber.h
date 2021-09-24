#pragma once
#include <random>

/// 随机数生成器
/// Usage:
/// static RandomNumber randomMachine(0.0, 1.0); // 设为静态或全局变量
/// double r1 = randomMachine;
/// double r2 = randomMachine();
/// ...
template<typename T = int>
class RandomNumber
{
public:
	RandomNumber(T l = std::numeric_limits<T>::min(),
				 T u = std::numeric_limits<T>::max()) :
		ud(l, u),
		generator(std::random_device{}())
	{}

	operator T() { return ud(generator); }
	T operator()() { return ud(generator); }

private:
	static auto traitFunc()
	{
		if constexpr (std::is_integral_v<T>)
			return std::uniform_int_distribution<T>{};
		else if constexpr (std::is_floating_point_v<T>)
			return std::uniform_real_distribution<T>{};
		else
			return void();
	}
	using Distribution = decltype(traitFunc());

	Distribution ud;
	std::mt19937 generator;
};
