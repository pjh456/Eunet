#ifndef INCLUDE_EUNET_PLATFORM_FD
#define INCLUDE_EUNET_PLATFORM_FD

#include <ostream>

#include "eunet/util/result.hpp"
#include "eunet/util/error.hpp"

namespace platform::fd
{
    class Fd;
    struct FdView;
    struct Pipe;

    /**
     * @brief 文件描述符 (RAII 封装)
     *
     * 独占管理底层文件描述符的生命周期。
     * 确保在对象析构时自动调用 close 关闭资源。
     * 禁止拷贝，只允许移动 (Move-only)。
     */
    class Fd
    {
    public:
        using FdResult = util::ResultV<Fd>;
        using PipeResult = util::ResultV<Pipe>;

    private:
        int fd;

    public:
        explicit Fd(int fd = -1) noexcept;
        ~Fd() noexcept;

        Fd(const Fd &) = delete;
        Fd &operator=(const Fd &) = delete;

        Fd(Fd &&other) noexcept;
        Fd &operator=(Fd &&other) noexcept;

    public:
        int get() const noexcept;
        bool valid() const noexcept;
        explicit operator bool() const noexcept;
        bool operator!() const noexcept;

    public:
        static FdResult
        socket(
            int domain,
            int type,
            int protocol) noexcept;

        FdView view() const noexcept;

        static PipeResult pipe() noexcept;

    public:
        int release() noexcept;
        void reset(int new_fd) noexcept;
    };

    /**
     * @brief 文件描述符视图
     *
     * 不持有所有权的文件描述符包装，仅用于参数传递。
     * 类似于 std::string_view 之于 std::string。
     * 使用者需自行确保 Fd 的持有者在 View 使用期间存活。
     */
    struct FdView
    {
        int fd;
        static FdView from_owner(const Fd &owner);
        explicit operator bool() const noexcept;

        bool operator==(const FdView &other) const noexcept;
    };

    struct Pipe
    {
        Fd read;
        Fd write;
    };
}

std::ostream &operator<<(
    std::ostream &os,
    const platform::fd::FdView fd);

#endif // INCLUDE_EUNET_PLATFORM_FD