#include "eunet/platform/capability.hpp"

#include <sstream>

namespace platform::capability
{
    namespace helper
    {
        cap_value_t to_linux_cap(Capability cap) noexcept
        {
            switch (cap)
            {
            case Capability::BindPrivilegedPort:
                return CAP_NET_BIND_SERVICE;
            case Capability::RawSocket:
            default:
                return CAP_NET_RAW;
                break;
            }
        }

        Capability linux_to_cap(cap_value_t cap) noexcept
        {
            switch (cap)
            {
            case CAP_NET_BIND_SERVICE:
                return Capability::BindPrivilegedPort;
            case CAP_NET_RAW:
            default:
                return Capability::RawSocket;
            }
        }

        util::Result<bool, CapabilityError>
        has_permitted(cap_value_t cap) noexcept
        {
            using Result = util::Result<bool, CapabilityError>;

            cap_t caps = cap_get_proc();
            if (!caps)
            {
                return Result::Err(CapabilityError{
                    .code = CapabilityErrorCode::GetProcCapsFailed,
                    .capability = linux_to_cap(cap),
                    .sys_errno = errno,
                    .syscall = "cap_get_proc",
                    .message = "failed to get process capabilities",
                    .hint = "check process privilege or libcap availability",
                });
            }

            cap_flag_value_t value{};
            if (cap_get_flag(caps, cap, CAP_PERMITTED, &value) != 0)
            {
                int err = errno;
                cap_free(caps);
                return Result::Err(CapabilityError{
                    .code = CapabilityErrorCode::GetFlagFailed,
                    .capability = linux_to_cap(cap),
                    .sys_errno = err,
                    .syscall = "cap_get_flag",
                    .message = "failed to query CAP_PERMITTED flag",
                });
            }

            cap_free(caps);
            return Result::Ok(value == CAP_SET);
        }

        util::Result<int, CapabilityError>
        set_effective(cap_value_t cap, bool enable) noexcept
        {
            using Result = util::Result<int, CapabilityError>;

            cap_t caps = cap_get_proc();
            if (!caps)
            {
                return Result::Err(CapabilityError{
                    .code = CapabilityErrorCode::GetProcCapsFailed,
                    .capability = linux_to_cap(cap),
                    .sys_errno = errno,
                    .syscall = "cap_get_proc",
                    .message = "failed to get process capabilities",
                });
            }

            cap_flag_value_t flag = enable ? CAP_SET : CAP_CLEAR;
            if (cap_set_flag(caps, CAP_EFFECTIVE, 1, &cap, flag) != 0)
            {
                int err = errno;
                cap_free(caps);
                return Result::Err(CapabilityError{
                    .code = CapabilityErrorCode::SetFlagFailed,
                    .capability = linux_to_cap(cap),
                    .sys_errno = err,
                    .syscall = "cap_set_flag",
                    .message = "failed to update CAP_EFFECTIVE flag",
                });
            }

            if (cap_set_proc(caps) != 0)
            {
                int err = errno;
                cap_free(caps);
                return Result::Err(CapabilityError{
                    .code = CapabilityErrorCode::SetProcFailed,
                    .capability = linux_to_cap(cap),
                    .sys_errno = err,
                    .syscall = "cap_set_proc",
                    .message = "failed to apply process capabilities",
                    .hint = "process may lack CAP_SETPCAP or be restricted by bounding set",
                });
            }

            cap_free(caps);
            return Result::Ok(1);
        }
    }

    std::string CapabilityError::to_string() const
    {
        std::ostringstream oss;
        oss << message;

        if (sys_errno)
            oss << " (errno=" << *sys_errno << ")";
        if (syscall)
            oss << " at " << *syscall;
        if (!hint.empty())
            oss << " | hint: " << hint;
        return oss.str();
    }

    CapabilityManager &CapabilityManager::instance()
    {
        static CapabilityManager manager;
        return manager;
    }

    CapabilityManager::CapabilityManager()
    {
        (void)drop_all_effective();
    }

    util::Result<CapabilityState, CapabilityError>
    CapabilityManager::state(Capability cap) const noexcept
    {
        using Result = util::Result<CapabilityState, CapabilityError>;

        auto res = helper::has_permitted(helper::to_linux_cap(cap));
        return res.is_err()
                   ? Result::Err(res.unwrap_err())
                   : Result::Ok(
                         res.unwrap()
                             ? CapabilityState::Available
                             : CapabilityState::Missing);
    }

    util::Result<int, CapabilityError>
    CapabilityManager::enable(Capability cap) noexcept
    {
        using Result = util::Result<int, CapabilityError>;

        cap_value_t linux_cap = helper::to_linux_cap(cap);

        auto permitted = helper::has_permitted(linux_cap);
        if (permitted.is_err())
            return Result::Err(permitted.unwrap_err());

        if (!permitted.unwrap())
        {
            return Result::Err(CapabilityError{
                .code = CapabilityErrorCode::NotPermitted,
                .capability = cap,
                .message = "capability not present in permitted set",
                .hint = "run as root or grant capability via setcap",
            });
        }

        return helper::set_effective(linux_cap, true);
    }

    util::Result<int, CapabilityError>
    CapabilityManager::disable(Capability cap) noexcept
    {
        return helper::set_effective(
            helper::to_linux_cap(cap), false);
    }

    util::Result<int, CapabilityError>
    CapabilityManager::drop_all_effective() noexcept
    {
        using Result = util::Result<int, CapabilityError>;

        cap_t caps = cap_get_proc();
        if (!caps)
        {
            return Result::Err(CapabilityError{
                .code = CapabilityErrorCode::GetProcCapsFailed,
                .capability = Capability::_Process,
                .sys_errno = errno,
                .syscall = "cap_get_proc",
                .message = "failed to get process capabilities",
            });
        }

        if (cap_clear_flag(caps, CAP_EFFECTIVE) != 0)
        {
            int err = errno;
            cap_free(caps);
            return Result::Err(CapabilityError{
                .code = CapabilityErrorCode::SetFlagFailed,
                .capability = Capability::RawSocket,
                .sys_errno = err,
                .syscall = "cap_clear_flag",
                .message = "failed to clear CAP_EFFECTIVE flags",
            });
        }

        if (cap_set_proc(caps) != 0)
        {
            int err = errno;
            cap_free(caps);
            return Result::Err(CapabilityError{
                .code = CapabilityErrorCode::SetProcFailed,
                .capability = Capability::RawSocket,
                .sys_errno = err,
                .syscall = "cap_set_proc",
                .message = "failed to apply cleared effective capabilities",
                .hint = "process may lack CAP_SETPCAP or be restricted by bounding set",
            });
        }

        cap_free(caps);
        return Result::Ok(1);
    }

    ScopedCapability::ScopedCapability(Capability cap) noexcept
        : cap(cap) {}

    ScopedCapability::
        ScopedCapability(ScopedCapability &&other) noexcept
        : cap(std::move(other.cap)) {}

    ScopedCapability::~ScopedCapability()
    {
        auto &manager = CapabilityManager::instance();
        auto res = manager.state(cap);
        if (res.is_err())
            return;
        if (res.unwrap() == CapabilityState::Available)
            (void)CapabilityManager::instance().disable(cap);
    }

    util::Result<ScopedCapability, CapabilityError>
    ScopedCapability::acquire(Capability cap) noexcept
    {
        using Result = util::Result<ScopedCapability, CapabilityError>;

        auto res = CapabilityManager::instance().enable(cap);
        if (res.is_err())
            return Result::Err(res.unwrap_err());

        return Result::Ok(ScopedCapability(cap));
    }
}