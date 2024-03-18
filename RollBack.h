#pragma once
#include <utility> // std::forward, std::decay_t

namespace RollBacks
{
template<typename F>
class Cleanup
{
public:
    template<typename Fi>
    Cleanup(Fi&& f) : f(std::forward<Fi>(f)), doClean(true) {}

    ~Cleanup() { if(doClean) f(); }

	Cleanup(Cleanup&&) = default;
	Cleanup& operator=(Cleanup&&) & = default;

	// 复制会导致多次 cleanup
	Cleanup(const Cleanup&) = delete;
	Cleanup& operator=(const Cleanup&) & = delete;

public:
    F f;
    bool doClean;
};

template<typename F> Cleanup(F&&) -> Cleanup<std::decay_t<F>>;

template<typename... Fs>
void resetCleans(Cleanup<Fs>&... clean)
{
	((clean.doClean = false), ...);
}
} // namespace RollBacks

// back compatibility
#ifndef UTILITY_NOT_USE_ROLLBACKS_NAMESPACE
using namespace RollBacks;
#endif
