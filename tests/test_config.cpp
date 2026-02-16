#include "test_utils.h"
#include "heidi-kernel/config.h"

#include <vector>
#include <string>

// Helper to convert std::vector<std::string> to argc, argv
class ArgBuilder {
public:
    ArgBuilder(std::vector<std::string> args) : args_(std::move(args)) {}

    int argc() const { return static_cast<int>(args_.size()); }
    char** argv() {
        if (argv_.empty()) {
            argv_.reserve(args_.size() + 1);
            for (auto& arg : args_) {
                argv_.push_back(const_cast<char*>(arg.data()));
            }
            argv_.push_back(nullptr);
        }
        return argv_.data();
    }

private:
    std::vector<std::string> args_;
    std::vector<char*> argv_;
};

TEST(ConfigParser, Default) {
    std::vector<std::string> args = {"heidi-kernel"};
    ArgBuilder builder(args);
    auto result = heidi::ConfigParser::parse(builder.argc(), builder.argv());
    ASSERT_TRUE(result.ok());
    const auto& config = result.value();
    ASSERT_EQ(config.log_level, "info");
    ASSERT_FALSE(config.show_version);
    ASSERT_FALSE(config.show_help);
}

TEST(ConfigParser, Help) {
    std::vector<std::string> args = {"heidi-kernel", "--help"};
    ArgBuilder builder(args);
    auto result = heidi::ConfigParser::parse(builder.argc(), builder.argv());
    ASSERT_TRUE(result.ok());
    ASSERT_TRUE(result.value().show_help);
}

TEST(ConfigParser, Version) {
    std::vector<std::string> args = {"heidi-kernel", "-v"};
    ArgBuilder builder(args);
    auto result = heidi::ConfigParser::parse(builder.argc(), builder.argv());
    ASSERT_TRUE(result.ok());
    ASSERT_TRUE(result.value().show_version);
}

TEST(ConfigParser, LogLevel) {
    std::vector<std::string> args = {"heidi-kernel", "--log-level", "debug"};
    ArgBuilder builder(args);
    auto result = heidi::ConfigParser::parse(builder.argc(), builder.argv());
    ASSERT_TRUE(result.ok());
    ASSERT_EQ(result.value().log_level, "debug");
}

TEST(ConfigParser, ConfigPath) {
    std::vector<std::string> args = {"heidi-kernel", "--config", "/etc/heidi/config.yaml"};
    ArgBuilder builder(args);
    auto result = heidi::ConfigParser::parse(builder.argc(), builder.argv());
    ASSERT_TRUE(result.ok());
    ASSERT_EQ(result.value().config_path, "/etc/heidi/config.yaml");
}

TEST(ConfigParser, InvalidArg) {
    std::vector<std::string> args = {"heidi-kernel", "--invalid"};
    ArgBuilder builder(args);
    auto result = heidi::ConfigParser::parse(builder.argc(), builder.argv());
    ASSERT_FALSE(result.ok());
    ASSERT_EQ(result.error().message, "Unknown argument");
}
