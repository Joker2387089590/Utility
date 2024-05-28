#pragma once
#include <functional>

namespace RollBacks
{
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
	std::function<void()> f;
    bool doClean;
};

template<typename... Cs>
void resetCleans(Cs&... clean) noexcept
{
	((clean.doClean = false), ...);
}
} // namespace RollBacks

// back compatibility
#ifndef UTILITY_NOT_USE_ROLLBACKS_NAMESPACE
using namespace RollBacks;
#endif
