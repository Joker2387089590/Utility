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

template<typename SysClock = system_clock, typename SteadyClock = high_resolution_clock>
struct DualTimePoint
{
	using System = SysClock;
	using Steady = SteadyClock;
	using Duration = typename Steady::duration;

	DualTimePoint() noexcept : system(System::now()), steady(Steady::now()) {}

	DualTimePoint(System::time_point system, Steady::time_point steady) noexcept :
		system(std::move(system)),
		steady(std::move(steady))
	{}

	DualTimePoint(const DualTimePoint&) noexcept = default;
	DualTimePoint& operator=(const DualTimePoint&) noexcept = default;
	DualTimePoint(DualTimePoint&&) noexcept = default;
	DualTimePoint& operator=(DualTimePoint&&) noexcept = default;

	void reset()
	{
		system = System::now();
		steady = Steady::now();
	}

	auto elapsed() const noexcept
	{
		auto xnow = Steady::now();
		auto duration = xnow - steady;
		auto snow = system + duration_cast<typename System::duration>(duration);
		return std::pair{DualTimePoint(snow, xnow), duration};
	}

	auto now() const noexcept
	{
		auto [point, duration] = elapsed();
		return point;
	}

	friend auto operator-(const DualTimePoint& a, const DualTimePoint& b) noexcept
	{
		return a.steady - b.steady;
	}

	// TODO: operators

#if !defined(UTILITY_TIMETOOL_NO_QDATETIME)
	QDateTime toQDateTime() const
	{
#   if QT_VERSION >= QT_VERSION_CHECK(6, 4, 0)
		return QDateTime::fromStdTimePoint(time_point_cast<milliseconds>(system));
#   else
		return QDateTime::fromMSecsSinceEpoch(system.time_since_epoch().count());
#   endif
	}
#endif

public:
	System::time_point system;
	Steady::time_point steady;
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
