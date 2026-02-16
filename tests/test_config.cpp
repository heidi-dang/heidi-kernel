#include "heidi-kernel/config.h"
#include <gtest/gtest.h>
#include <vector>
#include <string>

namespace heidi {
namespace {

class ConfigParserTest : public ::testing::Test {
protected:
    std::vector<char*> make_argv(const std::vector<std::string>& args) {
        std::vector<char*> argv;
        for (auto& arg : args) {
            argv.push_back(const_cast<char*>(arg.c_str()));
        }
        return argv;
    }
};

TEST_F(ConfigParserTest, ParseEmptyArgs) {
    std::vector<std::string> args = {"program"};
    auto argv = make_argv(args);
    
    auto result = ConfigParser::parse(args.size(), argv.data());
    
    ASSERT_TRUE(result.is_ok());
    auto config = result.unwrap();
    EXPECT_EQ(config.log_level, "info");
    EXPECT_FALSE(config.show_help);
    EXPECT_FALSE(config.show_version);
}

TEST_F(ConfigParserTest, ParseHelpFlag) {
    std::vector<std::string> args = {"program", "--help"};
    auto argv = make_argv(args);
    
    auto result = ConfigParser::parse(args.size(), argv.data());
    
    ASSERT_TRUE(result.is_ok());
    auto config = result.unwrap();
    EXPECT_TRUE(config.show_help);
}

TEST_F(ConfigParserTest, ParseVersionFlag) {
    std::vector<std::string> args = {"program", "--version"};
    auto argv = make_argv(args);
    
    auto result = ConfigParser::parse(args.size(), argv.data());
    
    ASSERT_TRUE(result.is_ok());
    auto config = result.unwrap();
    EXPECT_TRUE(config.show_version);
}

TEST_F(ConfigParserTest, ParseLogLevel) {
    std::vector<std::string> args = {"program", "--log-level", "debug"};
    auto argv = make_argv(args);
    
    auto result = ConfigParser::parse(args.size(), argv.data());
    
    ASSERT_TRUE(result.is_ok());
    auto config = result.unwrap();
    EXPECT_EQ(config.log_level, "debug");
}

TEST_F(ConfigParserTest, ParseConfigPath) {
    std::vector<std::string> args = {"program", "--config", "/path/to/config"};
    auto argv = make_argv(args);
    
    auto result = ConfigParser::parse(args.size(), argv.data());
    
    ASSERT_TRUE(result.is_ok());
    auto config = result.unwrap();
    EXPECT_EQ(config.config_path, "/path/to/config");
}

TEST_F(ConfigParserTest, ParseUnknownFlag) {
    std::vector<std::string> args = {"program", "--unknown-flag"};
    auto argv = make_argv(args);
    
    auto result = ConfigParser::parse(args.size(), argv.data());
    
    ASSERT_FALSE(result.is_ok());
    EXPECT_EQ(result.error().code, ErrorCode::InvalidArgument);
}

TEST_F(ConfigParserTest, ParseUnknownArgument) {
    std::vector<std::string> args = {"program", "unknown-arg"};
    auto argv = make_argv(args);
    
    auto result = ConfigParser::parse(args.size(), argv.data());
    
    ASSERT_FALSE(result.is_ok());
    EXPECT_EQ(result.error().code, ErrorCode::InvalidArgument);
}

TEST_F(ConfigParserTest, ParseShortFlags) {
    std::vector<std::string> args = {"program", "-h", "-v"};
    auto argv = make_argv(args);
    
    auto result = ConfigParser::parse(args.size(), argv.data());
    
    ASSERT_TRUE(result.is_ok());
    auto config = result.unwrap();
    EXPECT_TRUE(config.show_help);
    EXPECT_TRUE(config.show_version);
}

TEST_F(ConfigParserTest, Version) {
    EXPECT_EQ(ConfigParser::version(), "0.1.0");
}

} // namespace
} // namespace heidi
