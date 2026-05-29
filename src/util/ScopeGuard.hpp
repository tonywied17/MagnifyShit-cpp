#pragma once

#include <utility>

namespace magshit {

/// Runs a callable on scope exit unless `dismiss()` was called. Used to
/// pair RAII cleanup with C-style APIs (Win32, COM).
template <typename F>
class ScopeGuard
{
public:
    explicit ScopeGuard(F fn) : fn_(std::move(fn)) {}

    ~ScopeGuard()
    {
        if (active_)
        {
            fn_();
        }
    }

    /// Cancel the cleanup so the destructor becomes a no-op.
    void dismiss() noexcept { active_ = false; }

    ScopeGuard(const ScopeGuard&) = delete;
    ScopeGuard& operator=(const ScopeGuard&) = delete;
    ScopeGuard(ScopeGuard&& other) noexcept : fn_(std::move(other.fn_)), active_(other.active_)
    {
        other.active_ = false;
    }
    ScopeGuard& operator=(ScopeGuard&&) = delete;

private:
    F fn_;
    bool active_ = true;
};

template <typename F>
ScopeGuard(F) -> ScopeGuard<F>;

} // namespace magshit
