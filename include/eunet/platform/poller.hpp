#ifndef INCLUDE_EUNET_PLATFORM_POLLER
#define INCLUDE_EUNET_PLATFORM_POLLER

#include <sys/epoll.h>
#include <cstdint>
#include <vector>
#include <string>
#include <unordered_set>

#include "eunet/util/result.hpp"
#include "eunet/util/error.hpp"
#include "eunet/platform/fd.hpp"

namespace platform::poller
{
    enum class PollEventType : uint32_t
    {
        Read = EPOLLIN,
        Write = EPOLLOUT,
        Error = EPOLLERR | EPOLLHUP,
    };

    struct PollEvent;

    struct PollEvent
    {
        platform::fd::FdView fd;
        std::uint32_t events;

    public:
        bool is_readable() { return events & static_cast<uint32_t>(PollEventType::Read); }
        bool is_writable() { return events & static_cast<uint32_t>(PollEventType::Write); }
    };

    /**
     * @brief Epoll 多路复用器封装
     *
     * 封装 Linux epoll 相关的系统调用。
     * 提供面向对象的接口来管理关注的文件描述符事件。
     */
    class Poller
    {
    public:
        using FdTable = std::unordered_set<int>;

    private:
        static constexpr int MAX_EVENTS = 64;

    private:
        platform::fd::Fd epoll_fd;
        FdTable fd_table;

    public:
        static util::ResultV<Poller> create();

    private:
        Poller();

    public:
        Poller(const Poller &) = delete;
        Poller &operator=(const Poller &) = delete;

        Poller(Poller &&other) noexcept;
        Poller &operator=(Poller &&other) noexcept;

    public:
        bool valid() const noexcept;

        platform::fd::Fd &get_fd() noexcept;
        const platform::fd::Fd &get_fd() const noexcept;

        bool has_fd(int fd) const noexcept;

    public:
        /**
         * @brief 注册或更新感兴趣的事件
         *
         * @param fd 目标文件描述符视图
         * @param events 感兴趣的事件掩码 (如 EPOLLIN | EPOLLOUT)
         * @return ResultV<void> 成功或系统错误
         */
        util::ResultV<void> add(
            platform::fd::FdView fd,
            std::uint32_t events) noexcept;

        /**
         * @brief 注册或更新感兴趣的事件
         *
         * @param fd 目标文件描述符视图
         * @param events 感兴趣的事件掩码 (如 EPOLLIN | EPOLLOUT)
         * @return ResultV<void> 成功或系统错误
         */
        util::ResultV<void> modify(
            platform::fd::FdView fd,
            std::uint32_t events) noexcept;

        /**
         * @brief 移除感兴趣的事件
         *
         * @param fd 目标文件描述符视图
         * @return ResultV<void> 成功或系统错误
         */
        util::ResultV<void>
        remove(platform::fd::FdView fd) noexcept;

    public:
        /**
         * @brief 等待事件发生
         *
         * 封装 epoll_wait。
         *
         * @param timeout_ms 超时时间（毫秒），-1 表示无限等待
         * @return ResultV<std::vector<PollEvent>> 就绪的事件列表
         */
        util::ResultV<std::vector<PollEvent>>
        wait(int timeout_ms) noexcept;
    };
}

#endif // INCLUDE_EUNET_PLATFORM_POLLER