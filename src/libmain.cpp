/**
 * @author github.com/luncliff (luncliff@gmail.com)
 */
#include <coroutine/frame.h>

#include <chrono>
#include <spdlog/sinks/stdout_sinks.h>
#include <spdlog/spdlog.h>

#if defined(_WIN32)
#include <Windows.h>

namespace coro {

HMODULE global_dll_module = NULL;

std::shared_ptr<spdlog::logger> make_logger(const char* name, FILE* fout) noexcept(false);

} // namespace coro

BOOL APIENTRY DllMain(HANDLE handle, DWORD reason, LPVOID) {
    switch (reason) {
    case DLL_PROCESS_ATTACH:
        coro::global_dll_module = static_cast<HMODULE>(handle);
        break;
    default:
        return TRUE;
    }
    auto logger = coro::make_logger("coro", stdout);
    spdlog::set_default_logger(logger);
    return TRUE;
}

#endif

namespace coro {

std::shared_ptr<spdlog::logger> make_logger(const char* name, FILE* fout) noexcept(false) {
    using mutex_t = spdlog::details::console_nullmutex;
    using sink_t = spdlog::sinks::stdout_sink_base<mutex_t>;
    return std::make_shared<spdlog::logger>(name, std::make_shared<sink_t>(fout));
}

std::shared_ptr<spdlog::logger> get_log_stream() noexcept(false) {
    constexpr auto name = "coro";
    auto logger = spdlog::get(name);
    if (logger == nullptr) {
        logger = make_logger(name, stdout);
        spdlog::set_default_logger(logger);
    }
    return spdlog::get(name);
}

/**
 * @see C++ 20 <source_location>
 * 
 * @param loc Location hint to provide more information in the log
 * @param exp Exception caught in the coroutine's `unhandled_exception`
 */
void sink_exception(const spdlog::source_loc& loc, std::exception_ptr&& exp) noexcept {
    try {
        std::rethrow_exception(exp);
    } catch (const std::exception& ex) {
        spdlog::log(loc, spdlog::level::err, "{}", ex.what());
    } catch (...) {
        spdlog::critical("unknown exception type");
    }
}

}