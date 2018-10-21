// ---------------------------------------------------------------------------
//
//  Author
//      Park DongHa     | luncliff@gmail.com
//
//  License
//      CC BY 4.0
//
//  Note
//      Date time with Standard C++ headers
//
// ---------------------------------------------------------------------------
#ifndef _MAGIC_DATETIME_HPP_
#define _MAGIC_DATETIME_HPP_

#include <chrono>
#include <ctime>
#include <string>

namespace magic
{
// - Note
//      ISO-8601 Time
class date_time
{
  public:
    using clock_type = std::chrono::system_clock;
    using time_point = typename clock_type::time_point;

  private:
    time_point tp = clock_type::now();

  public:
    date_time() noexcept = default;

    explicit date_time(std::tm& time) noexcept
        : tp{clock_type::from_time_t(std::mktime(&time))}
    {
        // time_t t64 = std::mktime(&time);
        // tp = clock_type::from_time_t(t64);
    }
    explicit date_time(const std::time_t t64) noexcept
        : tp{clock_type::from_time_t(t64)}
    {
    }

  public:
    explicit operator time_point() const noexcept { return this->tp; }
    explicit operator std::tm() const noexcept
    {
        const std::time_t t64 = static_cast<std::time_t>(*this);
#ifdef _WIN32
        std::tm time{};
        gmtime_s(&time, &t64);
        return time;
#else
        return *gmtime(&t64);
#endif // _WIN32
    }
    explicit operator std::time_t() const noexcept
    {
        const std::time_t t64 = clock_type::to_time_t(this->tp);
        return t64;
    }

    // - Note
    //      Render to string with given string buffer. ISO 8601 format by
    //      default.
    void render(char* buffer,
                size_t capacity,
                const char* format = "%Y-%m-%d %H:%M:%S") const noexcept
    {
        const std::tm time = static_cast<std::tm>(*this);
        std::strftime(buffer, capacity, format, &time);
    }
};

} // namespace magic

#endif // _MAGIC_DATETIME_HPP_
