/*
 * ============================================================================
 *  File Name   : capability.cpp
 *  Module      : platform/capability
 *
 *  Description :
 *      Capability 实现。调用 libcap 库函数检查和设置进程的 Capabilities，
 *      实现提权和降权操作。
 *
 *  Third-Party Dependencies :
 *      - libcap
 *          Usage     : 系统 Capabilities 操作
 *          License   : BSD-style License / GPL
 *
 *  Author      : 爱特小登队
 *  Created On  : 2026-1-4
 *
 * ============================================================================
 */

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

        util::Result<bool, CapabilityErrorCode>
        has_permitted(cap_value_t cap) noexcept
        {
            using Result = util::Result<bool, CapabilityErrorCode>;

            cap_t caps = ::cap_get_proc();
            if (!caps)
                return Result::Err(
                    CapabilityErrorCode::GetProcCapsFailed);

            cap_flag_value_t value{};
            if (::cap_get_flag(caps, cap, CAP_PERMITTED, &value) != 0)
            {
                ::cap_free(caps);

                return Result::Err(CapabilityErrorCode::GetFlagFailed);
            }

            ::cap_free(caps);
            return Result::Ok(value == CAP_SET);
        }

        util::Result<void, CapabilityErrorCode>
        set_effective(cap_value_t cap, bool enable) noexcept
        {
            using Result = util::Result<void, CapabilityErrorCode>;

            cap_t caps = ::cap_get_proc();
            if (!caps)
                return Result::Err(CapabilityErrorCode::GetProcCapsFailed);

            cap_flag_value_t flag = enable ? CAP_SET : CAP_CLEAR;
            if (::cap_set_flag(caps, CAP_EFFECTIVE, 1, &cap, flag) != 0)
            {
                ::cap_free(caps);

                return Result::Err(CapabilityErrorCode::SetFlagFailed);
            }

            if (::cap_set_proc(caps) != 0)
            {
                ::cap_free(caps);

                return Result::Err(CapabilityErrorCode::SetProcFailed);
            }

            ::cap_free(caps);
            return Result::Ok();
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

    util::Result<CapabilityState, CapabilityErrorCode>
    CapabilityManager::state(Capability cap) const noexcept
    {
        using Result = util::Result<CapabilityState, CapabilityErrorCode>;

        auto res = helper::has_permitted(helper::to_linux_cap(cap));
        if (res.is_err())
            return Result::Err(res.unwrap_err());

        return Result::Ok(
            res.unwrap()
                ? CapabilityState::Available
                : CapabilityState::Missing);
    }

    util::Result<void, CapabilityErrorCode>
    CapabilityManager::enable(Capability cap) noexcept
    {
        using Result = util::Result<void, CapabilityErrorCode>;

        cap_value_t linux_cap = helper::to_linux_cap(cap);

        auto permitted = helper::has_permitted(linux_cap);
        if (permitted.is_err())
            return Result::Err(permitted.unwrap_err());

        if (!permitted.unwrap())
            return Result::Err(CapabilityErrorCode::NotPermitted);

        return helper::set_effective(linux_cap, true);
    }

    util::Result<void, CapabilityErrorCode>
    CapabilityManager::disable(Capability cap) noexcept
    {
        return helper::set_effective(
            helper::to_linux_cap(cap), false);
    }

    util::Result<void, CapabilityErrorCode>
    CapabilityManager::drop_all_effective() noexcept
    {
        using Result = util::Result<void, CapabilityErrorCode>;

        cap_t caps = ::cap_get_proc();
        if (!caps)
            return Result::Err(CapabilityErrorCode::GetProcCapsFailed);

        if (::cap_clear_flag(caps, CAP_EFFECTIVE) != 0)
        {
            ::cap_free(caps);

            return Result::Err(CapabilityErrorCode::SetFlagFailed);
        }

        if (::cap_set_proc(caps) != 0)
        {
            ::cap_free(caps);

            return Result::Err(CapabilityErrorCode::SetProcFailed);
        }

        ::cap_free(caps);
        return Result::Ok();
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

    util::Result<ScopedCapability, CapabilityErrorCode>
    ScopedCapability::acquire(Capability cap) noexcept
    {
        using Result = util::Result<ScopedCapability, CapabilityErrorCode>;

        auto res = CapabilityManager::instance().enable(cap);
        if (res.is_err())
            return Result::Err(res.unwrap_err());

        return Result::Ok(ScopedCapability(cap));
    }
}