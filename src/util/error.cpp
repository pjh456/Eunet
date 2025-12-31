#include "eunet/util/error.hpp"

#include <cstring>
#include <netdb.h>
#include <fmt/format.h>

namespace util
{
    std::string ErrorData::format() const
    {
        if (context.empty())
        {
            return fmt::format(
                "[{}]<{}>({}): {}",
                to_string(domain),
                to_string(category),
                code,
                message);
        }

        return fmt::format(
            "[{}]<{}>({}): {} [{}]",
            to_string(domain),
            to_string(category),
            code,
            message,
            context);
    }

    ErrorBuilder Error::create() { return ErrorBuilder(); }

    ErrorBuilder Error::dns() { return create().set_domain(ErrorDomain::DNS); }
    ErrorBuilder Error::transport() { return create().set_domain(ErrorDomain::Transport); }
    ErrorBuilder Error::security() { return create().set_domain(ErrorDomain::Security); }
    ErrorBuilder Error::protocol() { return create().set_domain(ErrorDomain::Protocol); }
    ErrorBuilder Error::system() { return create().set_domain(ErrorDomain::System); }
    ErrorBuilder Error::framework() { return create().set_domain(ErrorDomain::Framework); }

    Error Error::wrap(Error cause) const
    {
        Error e;
        e.m_cause = std::make_shared<Error>(std::move(cause));
        return e;
    }

    ErrorDomain Error::domain() const noexcept { return m_data ? m_data->domain : ErrorDomain::None; }
    int Error::code() const noexcept { return m_data ? m_data->code : 0; }
    std::string Error::message() const noexcept { return m_data ? m_data->message : "Success"; }
    const Error *Error::cause() const noexcept { return m_cause.get(); }

    std::string Error::format() const
    {
        if (is_ok())
            return "Success";

        std::string out = m_data->format();
        if (m_cause)
            out += " | Caused by: " + m_cause->format();
        return out;
    }

    void Error::wrap(std::shared_ptr<Error> err)
    {
        m_cause = err;
    }

    Error ErrorBuilder::build() const
    {
        auto data = std::make_shared<ErrorData>(
            ErrorData{
                m_domain,
                m_category,
                m_code,
                m_message.empty() ? "Unknown error" : m_message,
                m_context});

        Error err(data);
        if (m_cause)
            err.wrap(m_cause);

        return err;
    }
}

std::string_view to_string(util::ErrorDomain domain)
{
    using namespace util;
    switch (domain)
    {
    case ErrorDomain::DNS:
        return "DNS";
    case ErrorDomain::Transport:
        return "Transport";
    case ErrorDomain::Security:
        return "Security";
    case ErrorDomain::Protocol:
        return "Protocol";
    case ErrorDomain::System:
        return "System";
    case ErrorDomain::Framework:
        return "Framework";
    case ErrorDomain::None:
    default:
        return "None";
    }
}

std::string_view to_string(util::ErrorCategory category)
{
    using namespace util;
    switch (category)
    {
    case ErrorCategory::Timeout:
        return "Timeout";
    case ErrorCategory::Unreachable:
        return "Unreachable";
    case ErrorCategory::ConnectionRefused:
        return "ConnectionRefused";
    case ErrorCategory::ProtocolError:
        return "ProtocolError";
    case ErrorCategory::AuthFailed:
        return "AuthFailed";
    case ErrorCategory::ResourceExhausted:
        return "ResourceExhausted";
    case ErrorCategory::InvalidInput:
        return "InvalidInput";
    case ErrorCategory::Cancelled:
        return "Cancelled";
    case ErrorCategory::Unknown:
    default:
        return "Unknown";
    }
}