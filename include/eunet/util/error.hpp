#ifndef INCLUDE_EUNET_UTIL_ERROR
#define INCLUDE_EUNET_UTIL_ERROR

#include <string>
#include <string_view>
#include <memory>

#include "eunet/util/result.hpp"

namespace util
{
    /**
     * @brief 错误域定义
     *
     * 用于区分错误产生的逻辑层次或子系统。
     * 不同的域有助于在上层逻辑中快速定位问题源头。
     */
    enum class ErrorDomain
    {
        None = 0,

        // --- 网络栈 ---
        DNS,       // 名字解析 (getaddrinfo, c-ares)
        Transport, // L4 传输层 (TCP/UDP, socket API, epoll)
        Security,  // L5/L6 安全层 (TLS/SSL 握手, 证书校验)
        Protocol,  // L7 应用协议层 (HTTP 解析, WebSocket 帧)

        // --- 系统与环境 ---
        System,   // 操作系统底层 (File IO, Threading, Pipe)
        Hardware, // 硬件/网络接口 (网卡被拔出, Network unreachable)

        // --- 逻辑与配置 ---
        Config,  // 用户输入 URL 格式错误, 端口号溢出
        State,   // 状态机错误 (在 Connecting 状态调用 send)
        Internal // 库内部逻辑错误 (Assert 失败, 空指针)
    };

    /**
     * @brief 错误分类
     *
     * 定义错误的语义性质。这是业务逻辑判断如何处理错误的关键依据。
     * 例如：Busy 或 Timeout 通常意味着可以重试。
     */
    enum class ErrorCategory
    {
        Success = 0,
        Unknown,

        // --- 连接建立阶段 (Connectivity) ---
        Timeout,           // ETIMEDOUT: 经典的超时
        ConnectionRefused, // ECONNREFUSED: 对方主机在，但端口没开 (RST)
        HostUnreachable,   // EHOSTUNREACH: 路由不可达
        NetworkDown,       // ENETDOWN: 本地网络断开
        TargetNotFound,    // NXDOMAIN: 域名不存在 (DNS 特有)
        ResolutionFailed,  // SERVFAIL: DNS 服务器挂了 (不同于域名不存在)

        // --- 连接断开 (Disconnection) ---
        PeerClosed,      // EOF/FIN: 对端正常关闭 (read返回0)
        ConnectionReset, // ECONNRESET: 对端强制断开 (收到 RST)
        BrokenPipe,      // EPIPE: 向已关闭的连接写入
        Aborted,         // ECONNABORTED: 本地软件导致的中止

        // --- 协议与数据 (Data & Protocol) ---
        ProtocolViolation,  // 解析失败：HTTP 头格式不对
        PayloadTooLarge,    // 数据包超限：超过 buffer 大小
        UnsupportedVersion, // 协议版本不支持
        DataTruncated,      // 接收到的数据不完整

        // --- 安全与权限 (Security) ---
        AuthFailed,         // 401/403: 业务认证失败
        CertificateInvalid, // TLS: 证书过期、域名不匹配
        UntrustedAuthority, // TLS: 根证书不被信任

        // --- 资源与状态 (Resource & State) ---
        ResourceExhausted, // EMFILE/ENFILE: FD 耗尽, 内存不足
        Busy,              // EBUSY/EAGAIN: 资源暂时不可用 (非阻塞重试语义)
        InvalidState,      // 在错误的状态下执行了操作
        InvalidArgument,   // 参数非法 (用户输入错误)

        // --- 控制流 ---
        Cancelled // 用户点击了“停止”按钮
    };

    /**
     * @brief 错误严重程度
     *
     * 用于 UI 展示或日志分级的元数据。
     */
    enum class ErrorSeverity
    {
        Fatal,     // 红色：连接彻底失败，无法恢复 (NXDOMAIN, ConnectionRefused)
        Transient, // 黄色：临时错误，重试可能成功 (Timeout, Busy, ResolutionFailed)
        Logic      // 蓝色/灰色：配置错误或取消 (Cancelled, InvalidArgument)
    };

    class ErrorBuilder;
    class Error;

    /**
     * @brief 错误数据载体
     *
     * 存储错误的具体信息，包括错误码、描述文本和上下文。
     * 该结构体通常由 Error 类通过共享指针管理。
     */
    struct ErrorData
    {
        ErrorDomain domain = ErrorDomain::None;
        ErrorCategory category = ErrorCategory::Success;
        ErrorSeverity severity = ErrorSeverity::Logic;
        int code;
        std::string message;
        std::string context;

        std::string format() const;
    };

    /**
     * @brief 统一错误处理类
     *
     * 该类是系统中传递错误的核心载体。它采用值语义（内部引用计数），
     * 支持错误链（Caused By）机制，允许保留错误的原始触发原因。
     *
     * @note 该类设计为轻量级复制，可安全地在 Result<T, E> 中传递。
     */
    class Error
    {
    private:
        std::shared_ptr<const ErrorData> m_data;
        std::shared_ptr<Error> m_cause; // 错误链

    public:
        /**
         * @brief 创建一个错误构建器
         *
         * @return ErrorBuilder 用于链式构造 Error 对象
         */
        static ErrorBuilder create();

        /**
         * @brief 创建一个错误域为 DNS 的错误构建器
         *
         * @return ErrorBuilder 用于链式构造 Error 对象
         */
        static ErrorBuilder dns();

        /**
         * @brief 创建一个错误域为 transport 的错误构建器
         *
         * @return ErrorBuilder 用于链式构造 Error 对象
         */
        static ErrorBuilder transport();

        /**
         * @brief 创建一个错误域为 security 的错误构建器
         *
         * @return ErrorBuilder 用于链式构造 Error 对象
         */
        static ErrorBuilder security();

        /**
         * @brief 创建一个错误域为 protocol 的错误构建器
         *
         * @return ErrorBuilder 用于链式构造 Error 对象
         */
        static ErrorBuilder protocol();

        /**
         * @brief 创建一个错误域为 system 的错误构建器
         *
         * @return ErrorBuilder 用于链式构造 Error 对象
         */
        static ErrorBuilder system();

        /**
         * @brief 创建一个错误域为 hardware 的错误构建器
         *
         * @return ErrorBuilder 用于链式构造 Error 对象
         */
        static ErrorBuilder hardware();

        /**
         * @brief 创建一个错误域为 config 的错误构建器
         *
         * @return ErrorBuilder 用于链式构造 Error 对象
         */
        static ErrorBuilder config();

        /**
         * @brief 创建一个错误域为 state 的错误构建器
         *
         * @return ErrorBuilder 用于链式构造 Error 对象
         */
        static ErrorBuilder state();

        /**
         * @brief 创建一个错误域为 internal 的错误构建器
         *
         * @return ErrorBuilder 用于链式构造 Error 对象
         */
        static ErrorBuilder internal();

    public:
        Error() = default;
        explicit Error(std::shared_ptr<const ErrorData> data)
            : m_data(std::move(data)) {}

        Error(const Error &) = default;
        Error &operator=(const Error &) = default;

        Error(Error &&) noexcept = default;
        Error &operator=(Error &&) noexcept = default;

    public:
        /**
         * @brief 包装一个底层原因错误
         *
         * 包装一个错误，并将当前错误作为被包装错误的父级原因。
         *
         * @param cause 导致当前错误的原始错误
         */
        void wrap(std::shared_ptr<Error> err);

    public:
        bool is_ok() const noexcept { return !m_data; }
        explicit operator bool() const noexcept { return !is_ok(); }

    public:
        ErrorDomain domain() const noexcept;
        ErrorCategory category() const noexcept;
        int code() const noexcept;
        std::string message() const noexcept;
        const Error *cause() const noexcept;

        std::string format() const;
    };

    /**
     * @brief 错误构建器
     *
     * 采用 Builder 模式构造 Error 对象，允许流式设置属性。
     */
    class ErrorBuilder
    {
        friend class Error;

    private:
        ErrorDomain m_domain = ErrorDomain::None;
        ErrorCategory m_category = ErrorCategory::Unknown;
        ErrorSeverity m_severity = ErrorSeverity::Fatal;
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
        ErrorBuilder &hardware() { return set_domain(ErrorDomain::Hardware); }

        ErrorBuilder &config() { return set_domain(ErrorDomain::Config); }
        ErrorBuilder &state() { return set_domain(ErrorDomain::State); }
        ErrorBuilder &internal() { return set_domain(ErrorDomain::Internal); }

    public:
        ErrorBuilder &set_category(ErrorCategory cat)
        {
            m_category = cat;
            return *this;
        }

        ErrorBuilder &success() { return set_category(ErrorCategory::Success); }

        ErrorBuilder &timeout() { return set_category(ErrorCategory::Timeout); }
        ErrorBuilder &connection_refused() { return set_category(ErrorCategory::ConnectionRefused); }
        ErrorBuilder &host_unreachable() { return set_category(ErrorCategory::HostUnreachable); }
        ErrorBuilder &network_down() { return set_category(ErrorCategory::NetworkDown); }
        ErrorBuilder &target_not_found() { return set_category(ErrorCategory::TargetNotFound); }
        ErrorBuilder &resolution_failed() { return set_category(ErrorCategory::ResolutionFailed); }

        ErrorBuilder &peer_closed() { return set_category(ErrorCategory::PeerClosed); }
        ErrorBuilder &connection_reset() { return set_category(ErrorCategory::ConnectionReset); }
        ErrorBuilder &broken_pipe() { return set_category(ErrorCategory::BrokenPipe); }
        ErrorBuilder &aborted() { return set_category(ErrorCategory::Aborted); }

        ErrorBuilder &protocol_violation() { return set_category(ErrorCategory::ProtocolViolation); }
        ErrorBuilder &payload_too_large() { return set_category(ErrorCategory::PayloadTooLarge); }
        ErrorBuilder &unsupported_version() { return set_category(ErrorCategory::UnsupportedVersion); }
        ErrorBuilder &data_truncated() { return set_category(ErrorCategory::DataTruncated); }

        ErrorBuilder &auth_failed() { return set_category(ErrorCategory::AuthFailed); }
        ErrorBuilder &certificate_invalid() { return set_category(ErrorCategory::CertificateInvalid); }
        ErrorBuilder &untrusted_authority() { return set_category(ErrorCategory::UntrustedAuthority); }

        ErrorBuilder &resource_exhausted() { return set_category(ErrorCategory::ResourceExhausted); }
        ErrorBuilder &busy() { return set_category(ErrorCategory::Busy); }
        ErrorBuilder &invalid_state() { return set_category(ErrorCategory::InvalidState); }
        ErrorBuilder &invalid_argument() { return set_category(ErrorCategory::InvalidArgument); }

        ErrorBuilder &cancelled() { return set_category(ErrorCategory::Cancelled); }

    public:
        ErrorBuilder &set_severity(ErrorSeverity ser)
        {
            m_severity = ser;
            return *this;
        }

        ErrorBuilder &fatal() { return set_severity(ErrorSeverity::Fatal); }
        ErrorBuilder &transient() { return set_severity(ErrorSeverity::Transient); }
        ErrorBuilder &logic() { return set_severity(ErrorSeverity::Logic); }

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

util::ErrorCategory from_errno(int err_no);
util::ErrorCategory from_gai_error(int gai_err);

#endif // INCLUDE_EUNET_UTIL_ERROR