#include "eunet/platform/sys_error.hpp"

#include <errno.h>
#include <string.h>

namespace platform
{
    SysError SysError::from_errno(int err) noexcept
    {
        switch (err)
        {
        case 0:
            return {};

        case EACCES:
        case EPERM:
            return {err, SysErrorType::Permission};

        case ENOENT:
            return {err, SysErrorType::NotFound};

        case EINVAL:
        case EBADF:
            return {err, SysErrorType::Invalid};

        case EMFILE:
        case ENFILE:
        case ENOMEM:
            return {err, SysErrorType::Resource};

        case EINTR:
            return {err, SysErrorType::Interrupted};

        case EAGAIN:
            // case EWOULDBLOCK:
            return {err, SysErrorType::Again};

        case ENOSYS:
        case EAFNOSUPPORT:
            return {err, SysErrorType::Unsupported};

        default:
            if (err >= ECONNREFUSED && err <= ENETUNREACH)
                return {err, SysErrorType::Network};

            return {err, SysErrorType::Unknown};
        }
    }

    bool SysError::retryable() const noexcept
    {
        return type == SysErrorType::Interrupted ||
               type == SysErrorType::Again;
    }

    bool SysError::is_fatal() const noexcept
    {
        return !retryable();
    }

    const char *SysError::message() const noexcept
    {
        return code ? ::strerror(code) : "no error";
    }

}

const char *to_string(platform::SysErrorType type) noexcept
{
    using namespace platform;
    switch (type)
    {
    case SysErrorType::None:
        return "none";
    case SysErrorType::Permission:
        return "permission";
    case SysErrorType::NotFound:
        return "not_found";
    case SysErrorType::Invalid:
        return "invalid";
    case SysErrorType::Resource:
        return "resource";
    case SysErrorType::Interrupted:
        return "interrupted";
    case SysErrorType::Again:
        return "again";
    case SysErrorType::Network:
        return "network";
    case SysErrorType::Unsupported:
        return "unsupported";
    case SysErrorType::Unknown:
    default:
        return "unknown";
    }
}

std::string format_error(const platform::SysError &e)
{
    if (e.is_ok())
        return "[sys] ok";

    std::string out;
    out.reserve(128);

    out += "[sys] ";
    out += to_string(e.get_type());
    out += ": ";

    if (const char *msg = e.message(); msg && *msg)
        out += msg;
    else
        out += "unknown error";

    out += " (errno=";
    out += std::to_string(e.get_code());
    out += ")";

    return out;
}