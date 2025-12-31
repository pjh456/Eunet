#ifndef INCLUDE_EUNET_UTIL_ERROR
#define INCLUDE_EUNET_UTIL_ERROR

#include <string>
#include <string_view>
#include <memory>

#include "eunet/util/result.hpp"

namespace util
{
    enum class ErrorDomain
    {
        None = 0,
        DNS,       // 名字解析层
        Transport, // L4 传输层 (TCP/UDP)
        Security,  // 加密层 (TLS/SSL)
        Protocol,  // 应用协议层 (HTTP)
        System,    // 操作系统资源
        Framework  // 库内部逻辑
    };

    // 错误的语义性质（用于决定业务逻辑：是否重试、是否报错等）
    enum class ErrorCategory
    {
        Unknown = 0,
        Timeout,           // 超时
        Unreachable,       // 无法连接/找不到主机
        ConnectionRefused, // 被对端拒绝
        ProtocolError,     // 协议非法/解析失败
        AuthFailed,        // 认证失败
        AccessDenied,      // 权限不足
        ResourceExhausted, // 资源耗尽
        InvalidInput,      // 校验失败
        Cancelled          // 操作被取消
    };

    class ErrorBuilder;
    class Error;

    struct ErrorData
    {
        ErrorDomain domain;
        ErrorCategory category;
        int code;
        std::string message;
        std::string context;

        std::string format() const;
    };

    class Error
    {
    private:
        std::shared_ptr<const ErrorData> m_data;
        std::shared_ptr<Error> m_cause; // 错误链

    public:
        static ErrorBuilder create();

        static ErrorBuilder dns();
        static ErrorBuilder transport();
        static ErrorBuilder security();
        static ErrorBuilder protocol();
        static ErrorBuilder system();
        static ErrorBuilder framework();

    public:
        Error() = default;
        explicit Error(std::shared_ptr<const ErrorData> data)
            : m_data(std::move(data)) {}

        Error(const Error &) = default;
        Error &operator=(const Error &) = default;

        Error(Error &&) noexcept = default;
        Error &operator=(Error &&) noexcept = default;

    public:
        Error wrap(Error cause) const;

    public:
        bool is_ok() const noexcept { return !m_data; }
        explicit operator bool() const noexcept { return !is_ok(); }

    public:
        ErrorDomain domain() const noexcept;
        int code() const noexcept;
        std::string message() const noexcept;
        const Error *cause() const noexcept;

        std::string format() const;

    public:
        void wrap(std::shared_ptr<Error> err);
    };

    class ErrorBuilder
    {
        friend class Error;

    private:
        ErrorDomain m_domain = ErrorDomain::None;
        ErrorCategory m_category = ErrorCategory::Unknown;
        int m_code = 0;
        std::string m_message;
        std::string m_context; // 额外的上下文（如 syscall 名、URL 等）
        std::shared_ptr<Error> m_cause;

    private:
        ErrorBuilder() = default;

    public:
        ~ErrorBuilder() = default;

    public:
        Error build() const;

    public:
        ErrorBuilder &set_domain(ErrorDomain dom)
        {
            m_domain = dom;
            return *this;
        }

        ErrorBuilder &dns() { return set_domain(ErrorDomain::DNS); }
        ErrorBuilder &transport() { return set_domain(ErrorDomain::Transport); }
        ErrorBuilder &security() { return set_domain(ErrorDomain::Security); }
        ErrorBuilder &protocol() { return set_domain(ErrorDomain::Protocol); }
        ErrorBuilder &system() { return set_domain(ErrorDomain::System); }
        ErrorBuilder &framework() { return set_domain(ErrorDomain::Framework); }

    public:
        ErrorBuilder &set_category(ErrorCategory cat)
        {
            m_category = cat;
            return *this;
        }

        ErrorBuilder &timeout() { return set_category(ErrorCategory::Timeout); }
        ErrorBuilder &unreachable() { return set_category(ErrorCategory::Unreachable); }
        ErrorBuilder &connection_refused() { return set_category(ErrorCategory::ConnectionRefused); }
        ErrorBuilder &protocol_error() { return set_category(ErrorCategory::ProtocolError); }
        ErrorBuilder &auth_failed() { return set_category(ErrorCategory::AuthFailed); }
        ErrorBuilder &access_denied() { return set_category(ErrorCategory::AccessDenied); }
        ErrorBuilder &resource_exhausted() { return set_category(ErrorCategory::ResourceExhausted); }
        ErrorBuilder &invalid_input() { return set_category(ErrorCategory::InvalidInput); }
        ErrorBuilder &cancelled() { return set_category(ErrorCategory::Cancelled); }

    public:
        ErrorBuilder &message(std::string msg)
        {
            m_message = std::move(msg);
            return *this;
        }

        ErrorBuilder &code(int c)
        {
            m_code = c;
            return *this;
        }

        ErrorBuilder &context(std::string ctx)
        {
            m_context = std::move(ctx);
            return *this;
        }

    public:
        ErrorBuilder &wrap(Error cause)
        {
            m_cause = std::make_shared<Error>(cause);
            return *this;
        }
    };

    template <typename T>
    using ResultV = util::Result<T, Error>;
}

std::string_view to_string(util::ErrorDomain domain);
std::string_view to_string(util::ErrorCategory category);

#endif // INCLUDE_EUNET_UTIL_ERROR