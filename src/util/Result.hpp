#pragma once

#include <string>
#include <utility>
#include <variant>

namespace magshit {

/// @brief Lightweight error payload carried by `Result`.
struct Error
{
    std::string message;
    long code = 0;
};

/// @brief Either a value of type `T` or an `Error`.
/// @tparam T Successful payload type.
template <typename T>
class [[nodiscard]] Result
{
public:
    /**
     * @brief Construct a successful result.
     * @param value Payload to store.
     */
    Result(T value) : data_(std::move(value)) {}

    /**
     * @brief Construct a failed result.
     * @param err Error payload to store.
     */
    Result(Error err) : data_(std::move(err)) {}

    /**
     * @brief Check whether this result holds a value.
     * @return true when the result is successful.
     */
    bool ok() const noexcept { return std::holds_alternative<T>(data_); }

    /**
     * @brief Test success in boolean contexts.
     * @return true when the result is successful.
     */
    explicit operator bool() const noexcept { return ok(); }

    /**
     * @brief Access the mutable success value.
     * @return Stored payload.
     * @warning Undefined behavior if `!ok()`.
     */
    T& value() & { return std::get<T>(data_); }

    /**
     * @brief Access the immutable success value.
     * @return Stored payload.
     * @warning Undefined behavior if `!ok()`.
     */
    const T& value() const& { return std::get<T>(data_); }

    /**
     * @brief Move the success value out of the result.
     * @return Stored payload as an rvalue reference.
     * @warning Undefined behavior if `!ok()`.
     */
    T&& value() && { return std::get<T>(std::move(data_)); }

    /**
     * @brief Access the failure payload.
     * @return Stored error.
     * @warning Undefined behavior if `ok()`.
     */
    const Error& error() const& { return std::get<Error>(data_); }

private:
    std::variant<T, Error> data_;
};

/// @brief `Result` specialisation for operations that have no payload on success.
template <>
class [[nodiscard]] Result<void>
{
public:
    /**
     * @brief Construct a successful void result.
     */
    Result() = default;

    /**
     * @brief Construct a failed void result.
     * @param err Error payload to store.
     */
    Result(Error err) : err_(std::move(err)), failed_(true) {}

    /**
     * @brief Check whether this result represents success.
     * @return true when no error is stored.
     */
    bool ok() const noexcept { return !failed_; }

    /**
     * @brief Test success in boolean contexts.
     * @return true when no error is stored.
     */
    explicit operator bool() const noexcept { return ok(); }

    /**
     * @brief Access the failure payload.
     * @return Stored error.
     */
    const Error& error() const noexcept { return err_; }

private:
    Error err_{};
    bool failed_ = false;
};

/**
 * @brief Convenience factory for an `Error` value.
 * @param msg Human-readable error message.
 * @param code Optional platform or subsystem error code.
 * @return Error containing `msg` and `code`.
 */
inline Error makeError(std::string msg, long code = 0)
{
    return Error{std::move(msg), code};
}

} // namespace magshit
