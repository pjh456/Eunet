#include "eunet/util/error.hpp"

#include <cstring>
#include <netdb.h>
#include <fmt/format.h>

namespace util
{

    std::string ErrorImpl::format() const
    {
        return fmt::format(
            "[{}]({}): {}",
            to_string(domain()),
            code(), message());
    }

    class SysErrorImpl : public ErrorImpl
    {
    private:
        int m_errno;
        std::string m_syscall;

    public:
        SysErrorImpl(int e, std::string_view s)
            : m_errno(e), m_syscall(s) {}

    public:
        ErrorDomain domain() const override
        {
            return (m_errno == EMFILE || m_errno == ENFILE)
                       ? ErrorDomain::Resource
                       : ErrorDomain::System;
        }
        int code() const override { return m_errno; }
        std::string message() const override { return std::strerror(m_errno); }
        std::string format() const override
        {
            return m_syscall.empty()
                       ? message()
                       : fmt::format(
                             "syscall '{}' failed: {}",
                             m_syscall, message());
        }
    };

    class DnsErrorImpl : public ErrorImpl
    {
    private:
        int m_code;
        std::string m_host;

    public:
        DnsErrorImpl(int c, std::string_view h)
            : m_code(c), m_host(h) {}

    public:
        ErrorDomain domain() const override { return ErrorDomain::Network; }
        int code() const override { return m_code; }
        std::string message() const override { return gai_strerror(m_code); }
        std::string format() const override
        {
            return fmt::format(
                "DNS resolve failed for '{}': {}",
                m_host, message());
        }
    };

    class GenericErrorImpl : public ErrorImpl
    {
        ErrorDomain m_domain;
        int m_code;
        std::string m_msg;

    public:
        GenericErrorImpl(ErrorDomain d, int c, std::string m)
            : m_domain(d), m_code(c), m_msg(std::move(m)) {}
        ErrorDomain domain() const override { return m_domain; }
        int code() const override { return m_code; }
        std::string message() const override { return m_msg; }
    };

    // --- Error 包装器实现 ---

    Error Error::from_errno(int err_no, std::string_view syscall)
    {
        return Error(std::make_shared<SysErrorImpl>(
            err_no, syscall));
    }

    Error Error::from_gai(int gai_code, std::string_view host)
    {
        return Error(std::make_shared<DnsErrorImpl>(
            gai_code, host));
    }

    Error Error::internal(std::string msg)
    {
        return Error(std::make_shared<GenericErrorImpl>(
            ErrorDomain::Internal, -1, std::move(msg)));
    }

    Error Error::user(std::string msg)
    {
        return Error(std::make_shared<GenericErrorImpl>(
            ErrorDomain::User, -1, std::move(msg)));
    }

    ErrorDomain Error::domain() const noexcept { return m_impl ? m_impl->domain() : ErrorDomain::None; }
    int Error::code() const noexcept { return m_impl ? m_impl->code() : 0; }
    std::string Error::message() const noexcept { return m_impl ? m_impl->message() : "Success"; }
    const Error *Error::cause() const noexcept { return m_cause.get(); }

    std::string Error::format() const
    {
        if (is_ok())
            return "Success";

        std::string out = fmt::format(
            "[{}] {}",
            to_string(domain()),
            m_impl->format());

        if (m_cause)
            out += " | Caused by: " + m_cause->format();

        return out;
    }
}

std::string_view to_string(util::ErrorDomain domain)
{
    switch (domain)
    {
    case util::ErrorDomain::System:
        return "System";
    case util::ErrorDomain::Network:
        return "Network";
    case util::ErrorDomain::Transport:
        return "Transport";
    case util::ErrorDomain::Platform:
        return "Platform";
    case util::ErrorDomain::Resource:
        return "Resource";
    case util::ErrorDomain::Internal:
        return "Internal";
    case util::ErrorDomain::User:
        return "User";
    default:
        return "None";
    }
}