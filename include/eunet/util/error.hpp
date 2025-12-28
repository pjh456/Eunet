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
        System,   // OS 错误 (errno)
        Network,  // DNS (gai_error), TCP 协议栈
        Platform, // Poller, Capability 等平台组件
        Internal, // 内部逻辑错误
        User      // 用户输入或配置错误
    };

    class Error
    {
    private:
        ErrorDomain domain{ErrorDomain::None};
        int code{0};
        std::string message;
        std::shared_ptr<Error> cause; // 用于错误链

    public:
        static Error from_errno(int err_no, std::string_view context = "");
        static Error from_gai(int gai_code, std::string_view context = "");
        static Error internal(std::string_view msg);

    public:
        Error() = default;
        Error(ErrorDomain d, int c, std::string msg);

    public:
        ErrorDomain get_domain() const noexcept;
        int get_code() const noexcept;
        std::string_view get_message() const noexcept;
        std::shared_ptr<Error> get_cause() const noexcept;

    public:
        bool is_ok() const noexcept;
        explicit operator bool() const noexcept;

    public:
        // [System] Connection refused (errno: 111) | context: connect to 127.0.0.1
        std::string format() const;
    };

    template <typename T>
    using ResultV = util::Result<T, Error>;
}

std::string_view to_string(util::ErrorDomain domain);

#endif // INCLUDE_EUNET_UTIL_ERROR