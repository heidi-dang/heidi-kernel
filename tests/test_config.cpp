#include <gtest/gtest.h>
#include "heidi-kernel/config.h"

using namespace heidi;

TEST(ConfigParserTest, DefaultValues) {
    char* argv[] = { (char*)"program" };
    int argc = 1;

    auto result = ConfigParser::parse(argc, argv);
    ASSERT_TRUE(result.ok());

    const auto& config = result.value();
    EXPECT_EQ(config.log_level, "info");
    EXPECT_FALSE(config.show_help);
    EXPECT_FALSE(config.show_version);
    EXPECT_TRUE(config.config_path.empty());
}

TEST(ConfigParserTest, HelpFlag) {
    {
        char* argv[] = { (char*)"program", (char*)"--help" };
        int argc = 2;
        auto result = ConfigParser::parse(argc, argv);
        ASSERT_TRUE(result.ok());
        EXPECT_TRUE(result.value().show_help);
    }
    {
        char* argv[] = { (char*)"program", (char*)"-h" };
        int argc = 2;
        auto result = ConfigParser::parse(argc, argv);
        ASSERT_TRUE(result.ok());
        EXPECT_TRUE(result.value().show_help);
    }
}

TEST(ConfigParserTest, VersionFlag) {
    {
        char* argv[] = { (char*)"program", (char*)"--version" };
        int argc = 2;
        auto result = ConfigParser::parse(argc, argv);
        ASSERT_TRUE(result.ok());
        EXPECT_TRUE(result.value().show_version);
    }
    {
        char* argv[] = { (char*)"program", (char*)"-v" };
        int argc = 2;
        auto result = ConfigParser::parse(argc, argv);
        ASSERT_TRUE(result.ok());
        EXPECT_TRUE(result.value().show_version);
    }
}

TEST(ConfigParserTest, LogLevel) {
    char* argv[] = { (char*)"program", (char*)"--log-level", (char*)"debug" };
    int argc = 3;

    auto result = ConfigParser::parse(argc, argv);
    ASSERT_TRUE(result.ok());
    EXPECT_EQ(result.value().log_level, "debug");
}

TEST(ConfigParserTest, ConfigPath) {
    char* argv[] = { (char*)"program", (char*)"--config", (char*)"/etc/heidi.conf" };
    int argc = 3;

    auto result = ConfigParser::parse(argc, argv);
    ASSERT_TRUE(result.ok());
    EXPECT_EQ(result.value().config_path, "/etc/heidi.conf");
}

TEST(ConfigParserTest, MixedArgs) {
    char* argv[] = { (char*)"program", (char*)"--log-level", (char*)"warn", (char*)"--config", (char*)"config.yaml" };
    int argc = 5;

    auto result = ConfigParser::parse(argc, argv);
    ASSERT_TRUE(result.ok());
    EXPECT_EQ(result.value().log_level, "warn");
    EXPECT_EQ(result.value().config_path, "config.yaml");
}

TEST(ConfigParserTest, UnknownPositionalArg) {
    char* argv[] = { (char*)"program", (char*)"somefile" };
    int argc = 2;

    auto result = ConfigParser::parse(argc, argv);
    ASSERT_FALSE(result.ok());
    EXPECT_EQ(result.error().code, ErrorCode::InvalidArgument);
}

TEST(ConfigParserTest, IncompleteFlags) {
    // --log-level without value
    {
        char* argv[] = { (char*)"program", (char*)"--log-level" };
        int argc = 2;
        auto result = ConfigParser::parse(argc, argv);
        // It should be ignored or error?
        // Code: else if (arg == "--log-level" && i + 1 < argc)
        // If i+1 >= argc, it falls through.
        // It starts with "-", so it's ignored.
        // So log_level remains default.
        ASSERT_TRUE(result.ok());
        EXPECT_EQ(result.value().log_level, "info");
    }

     // --config without value
    {
        char* argv[] = { (char*)"program", (char*)"--config" };
        int argc = 2;
        auto result = ConfigParser::parse(argc, argv);
        // Similar to above.
        ASSERT_TRUE(result.ok());
        EXPECT_TRUE(result.value().config_path.empty());
    }
}

TEST(ConfigParserTest, UnknownFlagIgnored) {
    char* argv[] = { (char*)"program", (char*)"--unknown" };
    int argc = 2;

    auto result = ConfigParser::parse(argc, argv);
    // Based on code analysis, unknown flags starting with - are ignored.
    ASSERT_TRUE(result.ok());
}

TEST(ConfigParserTest, VersionString) {
    EXPECT_FALSE(ConfigParser::version().empty());
    EXPECT_EQ(ConfigParser::version(), "0.1.0");
}
