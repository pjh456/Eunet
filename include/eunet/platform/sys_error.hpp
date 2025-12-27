#ifndef INCLUDE_PLATFORM_SYS_ERROR
#define INCLUDE_PLATFORM_SYS_ERROR

#include <cstdint>

#include "eunet/util/result.hpp"

namespace platform
{
    enum class SysErrorType : uint8_t
    {
        None = 0,

        Permission,  // EACCES, EPERM
        NotFound,    // ENOENT
        Invalid,     // EINVAL, EBADF
        Resource,    // EMFILE, ENFILE, ENOMEM
        Interrupted, // EINTR
        Again,       // EAGAIN, EWOULDBLOCK
        Network,     // ECONN*, ENET*
        Unsupported, // EAFNOSUPPORT, ENOSYS
        Unknown,
    };

    class SysError
    {
    public:
        using code_type = int;

    private:
        code_type code; // 原始 errno
        SysErrorType type;

    public:
        constexpr SysError() noexcept
            : code(0), type(SysErrorType::None) {}

        constexpr SysError(code_type code, SysErrorType kind) noexcept
            : code(code), type(kind) {}

    public:
        static SysError from_errno(int err) noexcept;

    public:
        constexpr int get_code() const noexcept { return code; }
        constexpr SysErrorType get_type() const noexcept { return type; }

        constexpr bool is_ok() const noexcept { return code == 0; }
        explicit constexpr operator bool() const noexcept { return is_ok(); }

    public:
        bool retryable() const noexcept;
        bool is_fatal() const noexcept;

        const char *message() const noexcept;
    };

    template <typename T>
    using SysResult = util::Result<T, SysError>;
}

const char *to_string(platform::SysErrorType type) noexcept;

std::string format_error(const platform::SysError &e);

#endif // INCLUDE_PLATFORM_SYS_ERROR