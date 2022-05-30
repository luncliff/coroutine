#define CATCH_CONFIG_RUNNER
#include <catch2/catch.hpp>
#include <cstdio>
#include <spdlog/sinks/stdout_sinks.h>
#include <spdlog/spdlog.h>

auto make_logger(const char* name, FILE* fout) noexcept(false) {
    using mutex_t = spdlog::details::console_nullmutex;
    using sink_t = spdlog::sinks::stdout_sink_base<mutex_t>;
    return std::make_shared<spdlog::logger>(name, std::make_shared<sink_t>(fout));
}

auto logger = make_logger("test", stdout);

int main(int argc, char* argv[]) {
    logger->set_pattern("%T.%e [%L] %8t %v");
    logger->set_level(spdlog::level::level_enum::debug);
    spdlog::set_default_logger(logger);

    Catch::Session session{};
    session.applyCommandLine(argc, argv);
    return session.run();
}
