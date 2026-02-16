#include <gtest/gtest.h>
#include <sstream>
#include "heidi-kernel/logger.h"

using namespace heidi;

TEST(LoggerTest, DefaultConstructor) {
    std::stringstream ss;
    Logger logger(ss);
    EXPECT_EQ(logger.level(), LogLevel::Info);
}

TEST(LoggerTest, SetLevel) {
    std::stringstream ss;
    Logger logger(ss);
    logger.set_level(LogLevel::Debug);
    EXPECT_EQ(logger.level(), LogLevel::Debug);
}

TEST(LoggerTest, LogOutput) {
    std::stringstream ss;
    Logger logger(ss, LogLevel::Debug);

    logger.debug("debug msg");
    logger.info("info msg");
    logger.warn("warn msg");
    logger.error("error msg");

    std::string output = ss.str();
    EXPECT_TRUE(output.find("[DEBUG] debug msg") != std::string::npos);
    EXPECT_TRUE(output.find("[INFO] info msg") != std::string::npos);
    EXPECT_TRUE(output.find("[WARN] warn msg") != std::string::npos);
    EXPECT_TRUE(output.find("[ERROR] error msg") != std::string::npos);
}

TEST(LoggerTest, LogFiltering) {
    std::stringstream ss;
    Logger logger(ss, LogLevel::Warn);

    logger.debug("debug msg");
    logger.info("info msg");
    logger.warn("warn msg");
    logger.error("error msg");

    std::string output = ss.str();
    EXPECT_TRUE(output.find("[DEBUG] debug msg") == std::string::npos);
    EXPECT_TRUE(output.find("[INFO] info msg") == std::string::npos);
    EXPECT_TRUE(output.find("[WARN] warn msg") != std::string::npos);
    EXPECT_TRUE(output.find("[ERROR] error msg") != std::string::npos);
}

TEST(LoggerTest, LevelToString) {
    EXPECT_EQ(Logger::level_to_string(LogLevel::Debug), "DEBUG");
    EXPECT_EQ(Logger::level_to_string(LogLevel::Info), "INFO");
    EXPECT_EQ(Logger::level_to_string(LogLevel::Warn), "WARN");
    EXPECT_EQ(Logger::level_to_string(LogLevel::Error), "ERROR");
}
