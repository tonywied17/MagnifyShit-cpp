#pragma once

#include <utility>

namespace magshit {

/// @brief Runs a callable on scope exit unless `dismiss()` was called.
/// @tparam F Callable type invoked by the destructor while active.
template <typename F>
class ScopeGuard
{
public:
    /**
     * @brief Store the callable that should run on scope exit.
     * @param fn Cleanup callable.
     */
    explicit ScopeGuard(F fn) : fn_(std::move(fn)) {}

    /**
     * @brief Invoke the cleanup callable if the guard is still active.
     */
    ~ScopeGuard()
    {
        if (active_)
        {
            fn_();
        }
    }

    /**
     * @brief Cancel cleanup so the destructor becomes a no-op.
     */
    void dismiss() noexcept { active_ = false; }

    ScopeGuard(const ScopeGuard&) = delete;
    ScopeGuard& operator=(const ScopeGuard&) = delete;

    /**
     * @brief Move a scope guard and transfer cleanup responsibility.
     * @param other Guard to move from; it is dismissed by the move.
     */
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
