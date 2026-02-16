#pragma once

#include <cstdint>
#include <stdexcept>
#include <string_view>
#include <variant>

namespace heidi {

enum class ErrorCode : uint8_t {
    Success = 0,
    InvalidArgument = 1,
    ConfigNotFound = 2,
    ConfigParseError = 3,
    ShutdownRequested = 10,
    EventLoopError = 20,
    Unknown = 255,
};

struct Error {
    ErrorCode code;
    std::string_view message;

    [[nodiscard]] bool ok() const noexcept { return code == ErrorCode::Success; }
    explicit operator bool() const noexcept { return ok(); }
};

template<typename T>
class Result {
public:
    static Result ok(T value) {
        return Result(std::move(value));
    }

    static Result error(ErrorCode code, std::string_view msg) {
        return Result(Error{code, msg});
    }

    [[nodiscard]] bool ok() const noexcept { return std::holds_alternative<T>(data_); }
    explicit operator bool() const noexcept { return ok(); }

    [[nodiscard]] T& value() { return std::get<T>(data_); }
    [[nodiscard]] const T& value() const { return std::get<T>(data_); }
    [[nodiscard]] const Error& error() const { return std::get<Error>(data_); }

private:
    explicit Result(T value) : data_(std::move(value)) {}
    explicit Result(Error err) : data_(std::move(err)) {}

    std::variant<T, Error> data_;
};

template<>
class Result<void> {
public:
    static Result ok() { return Result(); }
    static Result error(ErrorCode code, std::string_view msg) {
        return Result(Error{code, msg});
    }

    [[nodiscard]] bool ok() const noexcept { return std::holds_alternative<std::monostate>(data_); }
    explicit operator bool() const noexcept { return ok(); }

    [[nodiscard]] const Error& error() const { return std::get<Error>(data_); }

private:
    explicit Result() : data_(std::monostate{}) {}
    explicit Result(Error err) : data_(std::move(err)) {}

    std::variant<std::monostate, Error> data_;
};

}
