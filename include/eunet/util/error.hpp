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
        System,    // 底层 OS 错误 (errno)
        Network,   // 名字解析 (DNS/GAI)
        Transport, // 协议栈错误 (TCP/UDP)
        Platform,  // 平台组件 (Poller/Epoll)
        Resource,  // 资源枯竭 (FD 不足, 内存溢出)
        Internal,  // 框架内部逻辑
        User       // 校验/配置错误
    };

    class ErrorImpl
    {
    public:
        virtual ~ErrorImpl() = default;
        virtual ErrorDomain domain() const = 0;
        virtual int code() const = 0;
        virtual std::string message() const = 0;
        virtual std::string format() const; // 默认格式化逻辑
    };

    class Error
    {
    private:
        std::shared_ptr<const ErrorImpl> m_impl;
        std::shared_ptr<Error> m_cause; // 错误链

    public:
        static Error from_errno(int err_no, std::string_view syscall = "");
        static Error from_gai(int gai_code, std::string_view host = "");
        static Error internal(std::string msg);
        static Error user(std::string msg);
        static Error platform(std::string msg, int code = -1);

    public:
        Error() = default;
        explicit Error(std::shared_ptr<const ErrorImpl> impl)
            : m_impl(std::move(impl)) {}

    public:
        Error &wrap(Error cause)
        {
            m_cause = std::make_shared<Error>(std::move(cause));
            return *this;
        }

    public:
        bool is_ok() const noexcept { return m_impl == nullptr; }
        explicit operator bool() const noexcept { return !is_ok(); }

    public:
        ErrorDomain domain() const noexcept;
        int code() const noexcept;
        std::string message() const noexcept;
        const Error *cause() const noexcept;

        std::string format() const;

    public:
        template <typename T>
        const T *as() const { return dynamic_cast<const T *>(m_impl.get()); }
    };

    template <typename T>
    using ResultV = util::Result<T, Error>;
}

std::string_view to_string(util::ErrorDomain domain);

#endif // INCLUDE_EUNET_UTIL_ERROR