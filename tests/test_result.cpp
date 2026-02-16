#include <catch2/catch_test_macros.hpp>

#include "heidi-kernel/result.h"

TEST_CASE("Result<int> ok value", "[result]") {
    auto result = heidi::Result<int>::ok(42);
    REQUIRE(result.ok() == true);
    REQUIRE(result.value() == 42);
}

TEST_CASE("Result<int> error", "[result]") {
    auto result = heidi::Result<int>::error(heidi::ErrorCode::InvalidArgument, "bad input");
    REQUIRE(result.ok() == false);
    REQUIRE(result.error().code == heidi::ErrorCode::InvalidArgument);
}

TEST_CASE("Result<void> success", "[result]") {
    auto result = heidi::Result<void>::success();
    REQUIRE(result.ok() == true);
}

TEST_CASE("Result<void> failure", "[result]") {
    auto result = heidi::Result<void>::failure(heidi::ErrorCode::ShutdownRequested, "stopping");
    REQUIRE(result.ok() == false);
    REQUIRE(result.error().code == heidi::ErrorCode::ShutdownRequested);
}
