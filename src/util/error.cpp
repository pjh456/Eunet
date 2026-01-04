/*
 * ============================================================================
 *  File Name   : error.cpp
 *  Module      : util
 *
 *  Description :
 *      Error 类的实现。包含错误信息的格式化逻辑、errno/gai_error 到内部
 *      ErrorCategory 的映射转换逻辑。
 *
 *  Third-Party Dependencies :
 *      - fmt
 *          Usage     : 格式化错误输出
 *          License   : MIT License
 *
 *  Author      : 爱特小登队
 *  Created On  : 2026-1-4
 *
 * ============================================================================
 */

#include "eunet/util/error.hpp"

#include <cstring>
#include <netdb.h>
#include <fmt/format.h>

#define DOMAIN_CASE(name)         \
    case util::ErrorDomain::name: \
        return #name

#define CATEGORY_CASE(name)         \
    case util::ErrorCategory::name: \
        return #name

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

    ErrorBuilder Error::dns() { return create().dns(); }
    ErrorBuilder Error::transport() { return create().transport(); }
    ErrorBuilder Error::security() { return create().security(); }
    ErrorBuilder Error::protocol() { return create().protocol(); }

    ErrorBuilder Error::system() { return create().system(); }
    ErrorBuilder Error::hardware() { return create().hardware(); }

    ErrorBuilder Error::config() { return create().config(); }
    ErrorBuilder Error::state() { return create().state(); }
    ErrorBuilder Error::internal() { return create().internal(); }

    ErrorDomain Error::domain() const noexcept { return m_data ? m_data->domain : ErrorDomain::None; }
    ErrorCategory Error::category() const noexcept { return m_data ? m_data->category : ErrorCategory::Unknown; }
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
                m_domain, m_category, m_severity,
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
    switch (domain)
    {
        DOMAIN_CASE(DNS);
        DOMAIN_CASE(Transport);
        DOMAIN_CASE(Security);
        DOMAIN_CASE(Protocol);

        DOMAIN_CASE(System);
        DOMAIN_CASE(Hardware);

        DOMAIN_CASE(Config);
        DOMAIN_CASE(State);
        DOMAIN_CASE(Internal);

    default:
        DOMAIN_CASE(None);
    }
}

std::string_view to_string(util::ErrorCategory category)
{
    switch (category)
    {
        CATEGORY_CASE(Success);

        CATEGORY_CASE(Timeout);
        CATEGORY_CASE(ConnectionRefused);
        CATEGORY_CASE(HostUnreachable);
        CATEGORY_CASE(NetworkDown);
        CATEGORY_CASE(TargetNotFound);
        CATEGORY_CASE(ResolutionFailed);

        CATEGORY_CASE(PeerClosed);
        CATEGORY_CASE(ConnectionReset);
        CATEGORY_CASE(BrokenPipe);
        CATEGORY_CASE(Aborted);

        CATEGORY_CASE(ProtocolViolation);
        CATEGORY_CASE(PayloadTooLarge);
        CATEGORY_CASE(UnsupportedVersion);
        CATEGORY_CASE(DataTruncated);

        CATEGORY_CASE(AuthFailed);
        CATEGORY_CASE(CertificateInvalid);
        CATEGORY_CASE(UntrustedAuthority);

        CATEGORY_CASE(ResourceExhausted);
        CATEGORY_CASE(Busy);
        CATEGORY_CASE(InvalidState);
        CATEGORY_CASE(InvalidArgument);

        CATEGORY_CASE(Cancelled);

    default:
        CATEGORY_CASE(Unknown);
    }
}

util::ErrorCategory from_errno(int err_no)
{
    using util::ErrorCategory;
    switch (err_no)
    {
    case ETIMEDOUT:
        return ErrorCategory::Timeout;
    case ECONNREFUSED:
        return ErrorCategory::ConnectionRefused;
    case ENETUNREACH:
    case EHOSTUNREACH:
        return ErrorCategory::HostUnreachable;
    case ENETDOWN:
        return ErrorCategory::NetworkDown;
    case EPIPE:
        return ErrorCategory::BrokenPipe;
    case ECONNRESET:
        return ErrorCategory::ConnectionReset;
    case ECONNABORTED:
        return ErrorCategory::Aborted;
    case EMFILE:
    case ENFILE:
    case ENOMEM:
        return ErrorCategory::ResourceExhausted;
    case EINVAL:
        return ErrorCategory::InvalidArgument;
    case EAGAIN:
        return ErrorCategory::Busy;
    default:
        return ErrorCategory::Unknown;
    }
}

util::ErrorCategory from_gai_error(int gai_err)
{
    using util::ErrorCategory;
    switch (gai_err)
    {
    case EAI_NONAME:
        return ErrorCategory::TargetNotFound; // 关键：域名不存在
    case EAI_AGAIN:
        return ErrorCategory::Busy; // 临时错误，可重试
    case EAI_FAIL:
        return ErrorCategory::ResolutionFailed; // 服务器错误
    case EAI_MEMORY:
        return ErrorCategory::ResourceExhausted;
    default:
        return ErrorCategory::Unknown;
    }
}