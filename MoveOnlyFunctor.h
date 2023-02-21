#pragma once
#include <functional>

template<typename F>
class MoveOnlyFunctor : public std::function<F>
{
public:
	using std::function<F>::function;
	MoveOnlyFunctor(const MoveOnlyFunctor&) = delete;
	MoveOnlyFunctor& operator=(const MoveOnlyFunctor&) = delete;
};
