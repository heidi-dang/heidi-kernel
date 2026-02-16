#include <catch2/catch_test_macros.hpp>
#include <sstream>

#include "heidi-kernel/logger.h"

TEST_CASE("Logger writes at correct level", "[logger]") {
    std::ostringstream oss;
    heidi::Logger logger{oss, heidi::LogLevel::Warn};

    logger.debug("debug message");
    logger.info("info message");
    logger.warn("warn message");
    logger.error("error message");

    std::string output = oss.str();
    REQUIRE(output.find("debug message") == std::string::npos);
    REQUIRE(output.find("info message") == std::string::npos);
    REQUIRE(output.find("warn message") != std::string::npos);
    REQUIRE(output.find("error message") != std::string::npos);
}

TEST_CASE("Logger level_to_string", "[logger]") {
    REQUIRE(heidi::Logger::level_to_string(heidi::LogLevel::Debug) == "DEBUG");
    REQUIRE(heidi::Logger::level_to_string(heidi::LogLevel::Info) == "INFO");
    REQUIRE(heidi::Logger::level_to_string(heidi::LogLevel::Warn) == "WARN");
    REQUIRE(heidi::Logger::level_to_string(heidi::LogLevel::Error) == "ERROR");
}
