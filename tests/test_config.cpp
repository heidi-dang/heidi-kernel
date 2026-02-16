#include <gtest/gtest.h>
#include <vector>
#include "heidi-kernel/config.h"

using namespace heidi;

namespace {

std::vector<char*> make_argv(const std::vector<std::string>& args) {
    std::vector<char*> argv;
    // We need to keep the strings alive, but here we assume args lives long enough
    // Actually, making a copy is safer.
    // But since we are inside a test function, we can just create a vector of char* pointing to string literals or local strings.
    // Let's use a simpler approach for each test.
    return argv;
}

}

TEST(ConfigParserTest, DefaultValues) {
    std::vector<char*> argv;
    char prog[] = "program";
    argv.push_back(prog);

    auto result = ConfigParser::parse(argv.size(), argv.data());

    ASSERT_TRUE(result.ok());
    const auto& config = result.value();
    EXPECT_EQ(config.log_level, "info");
    EXPECT_FALSE(config.show_help);
    EXPECT_FALSE(config.show_version);
    EXPECT_TRUE(config.config_path.empty());
}

TEST(ConfigParserTest, ShowHelpShort) {
    char prog[] = "program";
    char arg1[] = "-h";
    char* argv[] = {prog, arg1};

    auto result = ConfigParser::parse(2, argv);

    ASSERT_TRUE(result.ok());
    EXPECT_TRUE(result.value().show_help);
}

TEST(ConfigParserTest, ShowHelpLong) {
    char prog[] = "program";
    char arg1[] = "--help";
    char* argv[] = {prog, arg1};

    auto result = ConfigParser::parse(2, argv);

    ASSERT_TRUE(result.ok());
    EXPECT_TRUE(result.value().show_help);
}

TEST(ConfigParserTest, ShowVersionShort) {
    char prog[] = "program";
    char arg1[] = "-v";
    char* argv[] = {prog, arg1};

    auto result = ConfigParser::parse(2, argv);

    ASSERT_TRUE(result.ok());
    EXPECT_TRUE(result.value().show_version);
}

TEST(ConfigParserTest, ShowVersionLong) {
    char prog[] = "program";
    char arg1[] = "--version";
    char* argv[] = {prog, arg1};

    auto result = ConfigParser::parse(2, argv);

    ASSERT_TRUE(result.ok());
    EXPECT_TRUE(result.value().show_version);
}

TEST(ConfigParserTest, LogLevel) {
    char prog[] = "program";
    char arg1[] = "--log-level";
    char arg2[] = "debug";
    char* argv[] = {prog, arg1, arg2};

    auto result = ConfigParser::parse(3, argv);

    ASSERT_TRUE(result.ok());
    EXPECT_EQ(result.value().log_level, "debug");
}

TEST(ConfigParserTest, LogLevelMissingValue) {
    char prog[] = "program";
    char arg1[] = "--log-level";
    char* argv[] = {prog, arg1};

    auto result = ConfigParser::parse(2, argv);

    // Expect error because missing value implies the flag is unhandled or malformed
    EXPECT_FALSE(result.ok()) << "Missing value for --log-level should be an error";
}

TEST(ConfigParserTest, ConfigPath) {
    char prog[] = "program";
    char arg1[] = "--config";
    char arg2[] = "/etc/heidi.conf";
    char* argv[] = {prog, arg1, arg2};

    auto result = ConfigParser::parse(3, argv);

    ASSERT_TRUE(result.ok());
    EXPECT_EQ(result.value().config_path, "/etc/heidi.conf");
}

TEST(ConfigParserTest, ConfigPathMissingValue) {
    char prog[] = "program";
    char arg1[] = "--config";
    char* argv[] = {prog, arg1};

    auto result = ConfigParser::parse(2, argv);

    EXPECT_FALSE(result.ok()) << "Missing value for --config should be an error";
}

TEST(ConfigParserTest, UnknownFlag) {
    char prog[] = "program";
    char arg1[] = "--unknown";
    char* argv[] = {prog, arg1};

    auto result = ConfigParser::parse(2, argv);

    EXPECT_FALSE(result.ok()) << "Unknown flag should be an error";
    if (!result.ok()) {
         EXPECT_EQ(result.error().code, ErrorCode::InvalidArgument);
    }
}

TEST(ConfigParserTest, UnknownPositional) {
    char prog[] = "program";
    char arg1[] = "unknown";
    char* argv[] = {prog, arg1};

    auto result = ConfigParser::parse(2, argv);

    EXPECT_FALSE(result.ok());
    if (!result.ok()) {
         EXPECT_EQ(result.error().code, ErrorCode::InvalidArgument);
    }
}
