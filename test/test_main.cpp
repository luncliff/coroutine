#define CATCH_CONFIG_RUNNER
#if defined(_WIN32)
#define CATCH_CONFIG_WINDOWS_CRTDBG
#endif
#if __has_include(<catch2/catch_all.hpp>)
#include <catch2/catch_all.hpp>
#else
#include <catch2/catch.hpp>
#include <catch2/catch_reporter_sonarqube.hpp>
#endif
#include <cstdio>
#include <spdlog/sinks/stdout_sinks.h>
#include <spdlog/spdlog.h>

auto make_logger(const char* name, FILE* fout) noexcept(false) -> std::shared_ptr<spdlog::logger> {
    using mutex_t = spdlog::details::console_nullmutex;
    using sink_t = spdlog::sinks::stdout_sink_base<mutex_t>;
    if (fout == stdout)
        return spdlog::stdout_logger_st(name);
    return std::make_shared<spdlog::logger>(name, std::make_shared<sink_t>(fout));
}

int main(int argc, char* argv[]) {
    auto logger = make_logger("test", stdout);
    logger->set_pattern("%T.%e [%L] %8t %v");
    logger->set_level(spdlog::level::level_enum::debug);
    spdlog::set_default_logger(logger);

    Catch::Session session{};
    session.applyCommandLine(argc, argv);
    return session.run();
}
