#pragma once
#include <chrono>
#if !defined(UTILITY_TIMETOOL_NO_QDATETIME)
#  if __has_include(<QDateTime>)
#    include <QDateTime>
#  else
#    define UTILITY_TIMETOOL_NO_QDATETIME
#  endif
#endif

namespace TimeTool
{
using namespace std::chrono;

template<typename System = system_clock, typename Steady = high_resolution_clock>
struct DualTimePoint
{
    using Duration = typename Steady::duration;

    System::time_point system;
    Steady::time_point steady;

    DualTimePoint() noexcept : system(System::now()), steady(Steady::now()) {}

    DualTimePoint(System::time_point system, Steady::time_point steady) noexcept :
        system(std::move(system)),
        steady(std::move(steady))
    {}

    auto now() const noexcept
    {
        auto xnow = high_resolution_clock::now();
        auto duration = xnow - steady;
        return std::pair{DualTimePoint(system + duration, xnow), duration};
    }

    friend auto operator-(const DualTimePoint& a, const DualTimePoint& b) noexcept
    {
        return a.steady - b.steady;
    }

    // TODO: operators

#if !defined(UTILITY_TIMETOOL_NO_QDATETIME)
    QDateTime toQDateTime() const
    {
        return QDateTime::fromStdTimePoint(system);
    }
#endif
};

template<typename System, typename Steady>
DualTimePoint(System&&, Steady&&) -> DualTimePoint<std::decay_t<System>, std::decay_t<Steady>>;

inline auto secondsToHMS(std::intmax_t seconds) noexcept
{
    struct Diff
    {
		std::intmax_t hours{};
		std::intmax_t minutes{};
		std::intmax_t seconds{};
    };

	if(seconds == 0) return Diff{};

	int sign = seconds > 0 ? 1 : -1;
	auto [h, ms] = std::div(seconds * sign, std::intmax_t(60 * 60));
	auto [m, s]  = std::div(ms, std::intmax_t(60));
	return Diff{ h * sign, m, s };
}

inline auto msToHMSZ(std::intmax_t ms) noexcept
{
	struct Diff 
    {
		std::intmax_t hours{};
		std::intmax_t minutes{};
		std::intmax_t seconds{};
		std::intmax_t milliseconds{};
	};

	if(ms == 0) return Diff{};

	int sign = ms > 0 ? 1 : -1;
	auto [h, msz] = std::div(ms * sign, std::intmax_t(60 * 60 * 1000));
	auto [m, sz]  = std::div(msz, std::intmax_t(60 * 1000));
	auto [s, z]   = std::div(sz, std::intmax_t(1000));
	return Diff{ h * sign, m, s, z };
}

inline std::string formatHMSZ(std::intmax_t ms)
{
    std::string r;
    auto iter = std::back_inserter(r);
	auto [h, m, s, z] = msToHMSZ(ms);
    if(h != 0) std::format_to(iter, "{}h", h);
    if(m != 0 || !r.empty()) std::format_to(iter, "{}m", m);
    std::format_to(iter, "{:.3f}s", s + 0.001 * z);
    return r;
}
} // namespace TimeTool
