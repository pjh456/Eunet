#include "eunet/platform/capability.hpp"

#include <errno.h>

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

            cap_t caps = ::cap_get_proc();
            if (!caps)
            {
                return Result::Err(
                    CapabilityError{
                        CapabilityErrorCode::GetProcCapsFailed,
                        linux_to_cap(cap),
                        SysError::from_errno(errno),
                    });
            }

            cap_flag_value_t value{};
            if (::cap_get_flag(caps, cap, CAP_PERMITTED, &value) != 0)
            {
                SysError err = SysError::from_errno(errno);
                ::cap_free(caps);

                return Result::Err(
                    CapabilityError{
                        CapabilityErrorCode::GetFlagFailed,
                        linux_to_cap(cap),
                        err,
                    });
            }

            ::cap_free(caps);
            return Result::Ok(value == CAP_SET);
        }

        util::Result<bool, CapabilityError>
        set_effective(cap_value_t cap, bool enable) noexcept
        {
            using Result = util::Result<bool, CapabilityError>;

            cap_t caps = ::cap_get_proc();
            if (!caps)
            {
                return Result::Err(
                    CapabilityError{
                        CapabilityErrorCode::GetProcCapsFailed,
                        linux_to_cap(cap),
                        SysError::from_errno(errno),
                    });
            }

            cap_flag_value_t flag = enable ? CAP_SET : CAP_CLEAR;
            if (::cap_set_flag(caps, CAP_EFFECTIVE, 1, &cap, flag) != 0)
            {
                SysError err = SysError::from_errno(errno);
                ::cap_free(caps);

                return Result::Err(
                    CapabilityError{
                        CapabilityErrorCode::SetFlagFailed,
                        linux_to_cap(cap),
                        err,
                    });
            }

            if (::cap_set_proc(caps) != 0)
            {
                SysError err = SysError::from_errno(errno);
                ::cap_free(caps);

                return Result::Err(
                    CapabilityError{
                        CapabilityErrorCode::SetProcFailed,
                        linux_to_cap(cap),
                        err,
                    });
            }

            ::cap_free(caps);
            return Result::Ok(1);
        }
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
        if (res.is_err())
            return Result::Err(res.unwrap_err());

        return Result::Ok(
            res.unwrap()
                ? CapabilityState::Available
                : CapabilityState::Missing);
    }

    util::Result<bool, CapabilityError>
    CapabilityManager::enable(Capability cap) noexcept
    {
        using Result = util::Result<bool, CapabilityError>;

        cap_value_t linux_cap = helper::to_linux_cap(cap);

        auto permitted = helper::has_permitted(linux_cap);
        if (permitted.is_err())
            return Result::Err(permitted.unwrap_err());

        if (!permitted.unwrap())
        {
            return Result::Err(
                CapabilityError{
                    CapabilityErrorCode::NotPermitted,
                    cap,
                    SysError{},
                });
        }

        return helper::set_effective(linux_cap, true);
    }

    util::Result<bool, CapabilityError>
    CapabilityManager::disable(Capability cap) noexcept
    {
        return helper::set_effective(
            helper::to_linux_cap(cap), false);
    }

    util::Result<bool, CapabilityError>
    CapabilityManager::drop_all_effective() noexcept
    {
        using Result = util::Result<bool, CapabilityError>;

        cap_t caps = ::cap_get_proc();
        if (!caps)
        {
            return Result::Err(
                CapabilityError{
                    CapabilityErrorCode::GetProcCapsFailed,
                    Capability::_Process,
                    SysError::from_errno(errno),
                });
        }

        if (::cap_clear_flag(caps, CAP_EFFECTIVE) != 0)
        {
            SysError err = SysError::from_errno(errno);
            ::cap_free(caps);

            return Result::Err(
                CapabilityError{
                    CapabilityErrorCode::SetFlagFailed,
                    Capability::_Process,
                    err,
                });
        }

        if (::cap_set_proc(caps) != 0)
        {
            SysError err = SysError::from_errno(errno);
            ::cap_free(caps);

            return Result::Err(
                CapabilityError{
                    CapabilityErrorCode::SetProcFailed,
                    Capability::_Process,
                    err,
                });
        }

        ::cap_free(caps);
        return Result::Ok(1);
    }

    ScopedCapability::ScopedCapability(Capability cap) noexcept
        : cap(cap) {}

    ScopedCapability::
        ScopedCapability(ScopedCapability &&other) noexcept
        : cap(std::move(other.cap)) {}

    ScopedCapability::~ScopedCapability()
    {
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

std::string to_string(platform::capability::CapabilityErrorCode code)
{
    using namespace platform::capability;
    switch (code)
    {
    case CapabilityErrorCode::NotPermitted:
        return "not permitted";
    case CapabilityErrorCode::NotInBoundingSet:
        return "not in bounding set";

    case CapabilityErrorCode::GetProcCapsFailed:
        return "get proc caps failed";
    case CapabilityErrorCode::GetFlagFailed:
        return "get flag failed";
    case CapabilityErrorCode::SetFlagFailed:
        return "set flag failed";
    case CapabilityErrorCode::SetProcFailed:
        return "set proc failed";

    case CapabilityErrorCode::InvalidCapability:
        return "invalid capability";

    default:
        return "unknown";
    }
}

std::string format_error(const platform::capability::CapabilityError &e)
{
    std::string out = "[capability] ";
    out += to_string(e.code);

    if (!e.cause.is_ok())
    {
        out += " | ";
        out += format_error(e.cause);
    }

    return out;
}