#define CATCH_CONFIG_RUNNER
#include <catch2/catch.hpp>
#include <catch2/catch_reporter_sonarqube.hpp>
#include <cstdio>
#include <spdlog/spdlog.h>

std::shared_ptr<spdlog::logger> get_log_stream() noexcept(false);

int main(int argc, char* argv[]) {
    auto logger = get_log_stream();
    if (logger == nullptr)
        return EXIT_FAILURE;
    logger->set_pattern("%T.%e [%L] %8t %v");
    logger->set_level(spdlog::level::level_enum::debug);
    spdlog::set_default_logger(logger);

    Catch::Session session{};
    session.applyCommandLine(argc, argv);
    return session.run();
}
