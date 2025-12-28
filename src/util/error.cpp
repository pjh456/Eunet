#include "eunet/util/error.hpp"

#include <cstring>
#include <netdb.h>
#include <fmt/format.h>

namespace util
{
    Error Error::from_errno(int err_no, std::string_view context)
    {
        std::string msg = std::strerror(err_no);
        if (!context.empty())
        {
            msg = std::string(context) + ": " + msg;
        }
        return Error(ErrorDomain::System, err_no, std::move(msg));
    }

    Error Error::from_gai(int gai_code, std::string_view context)
    {
        std::string msg = gai_strerror(gai_code);
        if (!context.empty())
        {
            msg = std::string(context) + ": " + msg;
        }
        return Error(ErrorDomain::Network, gai_code, std::move(msg));
    }

    Error Error::internal(std::string_view msg)
    {
        return Error(ErrorDomain::Internal, -1, std::string(msg));
    }

    Error::Error(ErrorDomain d, int c, std::string msg)
        : domain(d), code(c), message(std::move(msg)) {}

    ErrorDomain Error::get_domain() const noexcept { return domain; }
    int Error::get_code() const noexcept { return code; }
    std::string_view Error::get_message() const noexcept { return message; }
    std::shared_ptr<Error> Error::get_cause() const noexcept { return cause; }

    bool Error::is_ok() const noexcept { return domain == ErrorDomain::None; }

    Error::operator bool() const noexcept { return !is_ok(); }

    std::string Error::format() const
    {
        if (is_ok())
            return "Success";

        return fmt::format(
            "[{}] {} ({}: {})",
            to_string(domain),
            message,
            (domain == ErrorDomain::System
                 ? "errno"
                 : "code"),
            code);
    }
}

std::string_view to_string(util::ErrorDomain domain)
{
    using namespace util;
    switch (domain)
    {
    case ErrorDomain::System:
        return "System";
    case ErrorDomain::Network:
        return "Network";
    case ErrorDomain::Platform:
        return "Platform";
    case ErrorDomain::Internal:
        return "Internal";
    case ErrorDomain::User:
        return "User";
    default:
        return "Unknown";
    }
}