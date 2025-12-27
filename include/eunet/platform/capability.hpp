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

    struct CapabilityError
    {
        CapabilityErrorCode code;
        Capability capability;

        // 系统级调试信息
        std::optional<int> sys_errno;
        std::optional<std::string> syscall;

        // 面向开发 / 用户
        std::string message; // 必填
        std::string hint;    // 可为空

        // 便于日志
        std::string to_string() const;
    };

    namespace helper
    {
        cap_value_t to_linux_cap(Capability cap) noexcept;
        Capability linux_to_cap(cap_value_t cap) noexcept;

        util::Result<bool, CapabilityError>
        has_permitted(cap_value_t cap) noexcept;

        util::Result<int, CapabilityError>
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
        util::Result<int, CapabilityError>
        enable(Capability cap) noexcept;
        util::Result<int, CapabilityError>
        disable(Capability cap) noexcept;

    public:
        util::Result<int, CapabilityError>
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

#endif // INCLUDE_EUNET_PLATFORM_CAPABILITY