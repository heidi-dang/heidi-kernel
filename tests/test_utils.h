#pragma once

#include <iostream>
#include <vector>
#include <functional>
#include <string>
#include <stdexcept>

struct TestCase {
    std::string name;
    std::function<void()> func;
};

inline std::vector<TestCase>& get_tests() {
    static std::vector<TestCase> tests;
    return tests;
}

inline void register_test(std::string name, std::function<void()> func) {
    get_tests().push_back({name, func});
}

#define TEST(suite, name) \
    void test_##suite##_##name(); \
    struct Register_##suite##_##name { \
        Register_##suite##_##name() { \
            register_test(#suite "." #name, test_##suite##_##name); \
        } \
    } register_##suite##_##name; \
    void test_##suite##_##name()

class TestFailure : public std::exception {
public:
    TestFailure(std::string msg) : msg_(msg) {}
    const char* what() const noexcept override { return msg_.c_str(); }
private:
    std::string msg_;
};

#define ASSERT_TRUE(condition) \
    if (!(condition)) { \
        throw TestFailure("Assertion failed: " #condition " at " + std::string(__FILE__) + ":" + std::to_string(__LINE__)); \
    }

#define ASSERT_EQ(a, b) \
    if ((a) != (b)) { \
        throw TestFailure("Assertion failed: " #a " == " #b " at " + std::string(__FILE__) + ":" + std::to_string(__LINE__)); \
    }

#define ASSERT_FALSE(condition) ASSERT_TRUE(!(condition))

inline int run_all_tests() {
    int passed = 0;
    int failed = 0;
    for (const auto& test : get_tests()) {
        std::cout << "[ RUN      ] " << test.name << std::endl;
        try {
            test.func();
            std::cout << "[       OK ] " << test.name << std::endl;
            passed++;
        } catch (const TestFailure& e) {
            std::cout << "[  FAILED  ] " << test.name << std::endl;
            std::cout << e.what() << std::endl;
            failed++;
        } catch (const std::exception& e) {
            std::cout << "[  FAILED  ] " << test.name << std::endl;
            std::cout << "Uncaught exception: " << e.what() << std::endl;
            failed++;
        } catch (...) {
            std::cout << "[  FAILED  ] " << test.name << std::endl;
            std::cout << "Unknown exception" << std::endl;
            failed++;
        }
    }
    std::cout << "[==========] " << (passed + failed) << " tests running." << std::endl;
    std::cout << "[  PASSED  ] " << passed << " tests." << std::endl;
    if (failed > 0) {
        std::cout << "[  FAILED  ] " << failed << " tests." << std::endl;
        return 1;
    }
    return 0;
}
