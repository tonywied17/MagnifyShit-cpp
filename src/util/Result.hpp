#pragma once

#include <string>
#include <utility>
#include <variant>

namespace magshit {

/// Lightweight error payload carried by `Result`.
struct Error
{
    std::string message;
    long code = 0;
};

/// Either a value of type `T` or an `Error`. Marked `[[nodiscard]]` so a
/// dropped Result is a warning.
template <typename T>
class [[nodiscard]] Result
{
public:
    Result(T value) : data_(std::move(value)) {}
    Result(Error err) : data_(std::move(err)) {}

    /// True when the Result holds a value.
    bool ok() const noexcept { return std::holds_alternative<T>(data_); }
    explicit operator bool() const noexcept { return ok(); }

    /// Access the value. Undefined behavior if `!ok()`.
    T& value() & { return std::get<T>(data_); }
    const T& value() const& { return std::get<T>(data_); }
    T&& value() && { return std::get<T>(std::move(data_)); }

    /// Access the error. Undefined behavior if `ok()`.
    const Error& error() const& { return std::get<Error>(data_); }

private:
    std::variant<T, Error> data_;
};

/// `Result` specialisation for operations that have no payload on success.
template <>
class [[nodiscard]] Result<void>
{
public:
    Result() = default;
    Result(Error err) : err_(std::move(err)), failed_(true) {}

    bool ok() const noexcept { return !failed_; }
    explicit operator bool() const noexcept { return ok(); }
    const Error& error() const noexcept { return err_; }

private:
    Error err_{};
    bool failed_ = false;
};

/// Convenience factory for an `Error` value.
inline Error makeError(std::string msg, long code = 0)
{
    return Error{std::move(msg), code};
}

} // namespace magshit
