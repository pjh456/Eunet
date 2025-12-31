#include "eunet/platform/io_context.hpp"

#include <sys/epoll.h>
#include <utility>

namespace platform
{
    IOContext::IOContext()
        : m_poller(poller::Poller::create().unwrap())
    {
    }

    void IOContext::run()
    {
        m_running = true;

        while (m_running)
        {
            auto evs = m_poller.wait(-1);
            if (evs.is_err())
                continue;

            for (auto &ev : evs.unwrap())
            {
                int fd = ev.fd.fd;
                auto it = m_states.find(fd);
                if (it == m_states.end())
                    continue;

                auto &state = it->second;

                if ((ev.events & EPOLLIN) && state.read_waiter)
                {
                    auto h = state.read_waiter;
                    state.read_waiter = {};

                    h.resume();
                }
            }
        }
    }

    IOResult
    IOContext::read(
        fd::Fd &fd,
        util::ByteBuffer &buf)
    {
        auto buf_len = buf.writable_size();
        auto buffer = buf.prepare(buf_len);
        ssize_t n = ::read(
            fd.get(),
            buffer.data(),
            buf_len);
        if (n < 0)
        {
            int err_no = errno;
            return IOResult::Err(
                util::Error::system()
                    .code(err_no)
                    .build());
        }
        buf.commit(n);
        return IOResult::Ok(n);
    }

    IOResult
    IOContext::write(
        fd::Fd &fd,
        util::ByteBuffer &buf)
    {
        auto data_len = buf.size();
        auto data = buf.readable();
        ssize_t n = ::write(
            fd.get(),
            data.data(),
            data_len);
        if (n < 0)
        {
            int err_no = errno;
            return IOResult::Err(
                util::Error::system()
                    .code(err_no)
                    .build());
        }
        buf.consume(n);
        return IOResult::Ok(n);
    }

    ReadAwaitable IOContext::async_read(
        fd::Fd &fd, util::ByteBuffer &buf)
    {
        return ReadAwaitable(*this, fd, buf);
    }

    ReadAwaitable::ReadAwaitable(
        IOContext &ctx, fd::Fd &fd,
        util::ByteBuffer &buf)
        : m_ctx(ctx), m_fd(fd),
          m_buf(buf),
          result(IOResult::Err(util::Error{})) {}

    bool ReadAwaitable::await_ready()
        const noexcept { return false; }

    void ReadAwaitable::await_suspend(
        std::coroutine_handle<> h)
    {
        auto &state = m_ctx.fd_state()[m_fd.get()];
        state.read_waiter = h;

        (void)m_ctx.poller().add(
            m_fd,
            EPOLLIN | EPOLLONESHOT);
    }

    IOResult
    ReadAwaitable::await_resume()
    {
        auto writable = m_buf.writable_size();
        auto span = m_buf.prepare(writable);

        ssize_t n = ::read(m_fd.get(), span.data(), writable);
        if (n < 0)
        {
            int err_no = errno;
            return IOResult::Err(
                util::Error::system()
                    .code(err_no)
                    .build());
        }

        m_buf.commit(n);
        return IOResult::Ok(n);
    }

}