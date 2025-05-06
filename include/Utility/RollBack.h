#pragma once
#include <functional>
#include "Macros.h"

namespace RollBacks
{
class Cleanup
{
public:
	explicit Cleanup() : f{}, doClean(false) {}

    template<typename Fi>
    Cleanup(Fi&& f) : f(std::forward<Fi>(f)), doClean(true) {}

	~Cleanup() { if(doClean && f) f(); }

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
UTILITY_DISABLE_WARNING_HEADER_HYGIENE
UTILITY_DISABLE_WARNING_PUSH
using namespace RollBacks;
UTILITY_DISABLE_WARNING_POP
#endif

#include "MacrosUndef.h"
