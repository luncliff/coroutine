// ---------------------------------------------------------------------------
//
//  Author  : github.com/luncliff (luncliff@gmail.com)
//  License : CC BY 4.0
//
//  Note
//      Simple stopwatch
//
// ---------------------------------------------------------------------------
#pragma once
#include <chrono>

// - Note
//      Stop watch.
template <typename Clock>
class stop_watch
{
  public:
    using clock_type = Clock;
    using time_point = typename clock_type::time_point;
    using duration = typename clock_type::duration;

  private:
    time_point start = clock_type::now();

  public:
    // - Note
    //    Acquire elapsed time from last reset
    template <typename UnitType = std::chrono::milliseconds>
    decltype(auto) pick() const noexcept
    {
        const duration span = clock_type::now() - start;
        return std::chrono::duration_cast<UnitType>(span);
    };

    // - Note
    //    Acquire elapsed time from last reset
    //    Then reset the starting time_point
    template <typename UnitType = std::chrono::milliseconds>
    decltype(auto) reset() noexcept
    {
        const auto span = this->pick<UnitType>();
        start = clock_type::now(); // reset start
        return span;
    }
};
