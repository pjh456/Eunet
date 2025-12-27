#ifndef INCLUDE_EUNET_PLATFORM_CAPABILITY
#define INCLUDE_EUNET_PLATFORM_CAPABILITY

#include <string>
#include <optional>

#include <sys/capability.h>
#include <unistd.h>

#include "eunet/util/result.hpp"
#include "eunet/platform/sys_error.hpp"

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

    struct CapabilityError
    {
        CapabilityErrorCode code;
        Capability capability;
        SysError cause;
    };

    namespace helper
    {
        cap_value_t to_linux_cap(Capability cap) noexcept;
        Capability linux_to_cap(cap_value_t cap) noexcept;

        util::Result<bool, CapabilityError>
        has_permitted(cap_value_t cap) noexcept;

        util::Result<bool, CapabilityError>
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
        util::Result<CapabilityState, CapabilityError>
        state(Capability cap) const noexcept;
        util::Result<bool, CapabilityError>
        enable(Capability cap) noexcept;
        util::Result<bool, CapabilityError>
        disable(Capability cap) noexcept;

    public:
        util::Result<bool, CapabilityError>
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
        static util::Result<ScopedCapability, CapabilityError>
        acquire(Capability cap) noexcept;
    };
}

std::string to_string(platform::capability::CapabilityErrorCode code);

#endif // INCLUDE_EUNET_PLATFORM_CAPABILITY