/*
 * ============================================================================
 *  File Name   : capability.hpp
 *  Module      : platform/capability
 *
 *  Description :
 *      Linux Capabilities (libcap) 的封装。
 *      用于细粒度的权限管理（如 CAP_NET_RAW 用于原始套接字），
 *      支持动态申请和释放特权，提供 RAII 风格的 ScopedCapability。
 *
 *  Third-Party Dependencies :
 *      - libcap
 *          Usage     : 系统 Capabilities 管理接口 (cap_get_proc 等)
 *          License   : BSD-style License / GPL
 *
 *  Author      : 爱特小登队
 *  Created On  : 2026-1-4
 *
 * ============================================================================
 */

#ifndef INCLUDE_EUNET_PLATFORM_CAPABILITY
#define INCLUDE_EUNET_PLATFORM_CAPABILITY

#include <string>
#include <optional>

#include <sys/capability.h>
#include <unistd.h>

#include "eunet/util/result.hpp"

namespace platform::capability
{

    enum class Capability
    {
        RawSocket,          // ICMP / raw socket
        BindPrivilegedPort, // <1024

        _Process, // 作用于整个进程（非具体 capability）
    };

    enum class CapabilityState
    {
        Available, // 在 permitted 集合中
        Missing,   // 不在 permitted 集合
        Unknown,   // 查询失败
    };

    enum class CapabilityErrorCode
    {
        // 权限与状态类
        NotPermitted,     // 不在 permitted 集合
        NotInBoundingSet, // 被 bounding set 限制（推断）

        // 系统调用失败
        GetProcCapsFailed, // cap_get_proc
        GetFlagFailed,     // cap_get_flag
        SetFlagFailed,     // cap_set_flag
        SetProcFailed,     // cap_set_proc

        // 逻辑错误
        InvalidCapability, // 未映射的 Capability
    };

    namespace helper
    {
        cap_value_t to_linux_cap(Capability cap) noexcept;
        Capability linux_to_cap(cap_value_t cap) noexcept;

        util::Result<bool, CapabilityErrorCode>
        has_permitted(cap_value_t cap) noexcept;

        util::Result<void, CapabilityErrorCode>
        set_effective(cap_value_t cap, bool enable) noexcept;
    }

    class CapabilityManager
    {
    public:
        static CapabilityManager &instance();

    private:
        CapabilityManager();
        ~CapabilityManager() = default;

    public:
        CapabilityManager(const CapabilityManager &) = delete;
        CapabilityManager &operator=(const CapabilityManager &) = delete;

        CapabilityManager(CapabilityManager &&) noexcept = delete;
        CapabilityManager &operator=(CapabilityManager &&) noexcept = delete;

    public:
        util::Result<CapabilityState, CapabilityErrorCode>
        state(Capability cap) const noexcept;
        util::Result<void, CapabilityErrorCode>
        enable(Capability cap) noexcept;
        util::Result<void, CapabilityErrorCode>
        disable(Capability cap) noexcept;

    public:
        util::Result<void, CapabilityErrorCode>
        drop_all_effective() noexcept;
    };

    class ScopedCapability
    {
    private:
        Capability cap;

    private:
        explicit ScopedCapability(Capability cap) noexcept;

    public:
        ScopedCapability(const ScopedCapability &) = delete;
        ScopedCapability &operator=(const ScopedCapability &) = delete;

        ScopedCapability(ScopedCapability &&other) noexcept;
        ScopedCapability &operator=(ScopedCapability &&other) noexcept = delete;

        ~ScopedCapability();

    public:
        static util::Result<ScopedCapability, CapabilityErrorCode>
        acquire(Capability cap) noexcept;
    };
}

#endif // INCLUDE_EUNET_PLATFORM_CAPABILITY