#pragma once
#include <limits>

namespace NumRanges
{
enum Compare : int {
    Less = -1,
    Equal = 0,
    Greater = 1
};

template<typename T>
struct NumRange
{
    using Limit = std::numeric_limits<T>;
    static constexpr T pInf = Limit::has_infinity ? +Limit::infinity() : Limit::max();
    static constexpr T nInf = Limit::has_infinity ? -Limit::infinity() : Limit::min();
    
    constexpr NumRange(T lower, T upper) noexcept : lower(lower), upper(upper) {}
    ~NumRange() noexcept = default;

    static constexpr NumRange greater(T lower) noexcept { return {lower, pInf}; }
    static constexpr NumRange less(T upper) noexcept { return {nInf, upper}; }

    constexpr bool inRange(T value) const noexcept { return lower <= value && value <= upper; }
    constexpr bool operator()(T value) const noexcept { return inRange(value); }

    constexpr Compare compare(T value) const noexcept
    {
        if(value < lower)
            return Less;
        else if(value <= upper)
            return Equal;
        else
            return Greater;
    }

    T lower;
    T upper;
};

inline constexpr double inf = std::numeric_limits<double>::infinity();
}
