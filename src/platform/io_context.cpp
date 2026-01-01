#include "eunet/platform/io_context.hpp"

#include <sys/epoll.h>
#include <utility>

namespace platform
{
    util::ResultV<IOContext>
    IOContext::create()
    {
        using Ret = util::ResultV<IOContext>;
        using util::Error;

        try
        {
            return Ret::Ok(IOContext());
        }
        catch (std::exception e)
        {
            return Ret::Err(
                Error::system()
                    .message(e.what())
                    .build());
        }
    }

    IOContext::IOContext()
        : m_poller(poller::Poller::create().unwrap())
    {
    }

    IOResult
    IOContext::read(
        net::BaseSocket &sock,
        util::ByteBuffer &buf,
        time::Duration timeout)
    {
        using util::Error;

        auto ddl = time::deadline_after(timeout);
        size_t bytes = 0;

        while (buf.writable_size() > 0)
        {
            auto r = sock.try_read(buf);
            if (r.is_err())
            {
                auto err = r.unwrap_err();
                if (err.category() != util::ErrorCategory::PeerClosed)
                {
                    return IOResult::Err(
                        Error::transport()
                            .message("socket read failed")
                            .wrap(err)
                            .build());
                }
                else
                    break;
            }

            auto n = r.unwrap();
            if (n > 0)
            {
                bytes += n;
                continue;
            }

            if (time::expired(ddl))
            {
                if (bytes > 0)
                    break;
                else
                    return IOResult::Err(
                        Error::transport()
                            .timeout()
                            .message("read time out")
                            .build());
            }

            auto ddl_ms =
                std::chrono::duration_cast<
                    std::chrono::milliseconds>(
                    time::elapsed(
                        time::monotonic_now(), ddl))
                    .count();

            auto wait_res = wait_readable(sock, ddl_ms);
            if (wait_res.is_err())
            {
                return IOResult::Err(
                    Error::system()
                        .message("epoll wait fd failed")
                        .wrap(wait_res.unwrap_err())
                        .build());
            }
        }

        return IOResult::Ok(bytes);
    }

    IOResult
    IOContext::write(
        net::BaseSocket &sock,
        util::ByteBuffer &buf,
        time::Duration timeout)
    {
        using util::Error;

        auto ddl = time::deadline_after(timeout);
        size_t bytes = 0;

        while (buf.size() > 0)
        {
            auto r = sock.try_write(buf);
            if (r.is_err())
            {
                auto err = r.unwrap_err();
                if (err.category() != util::ErrorCategory::PeerClosed)
                {
                    return IOResult::Err(
                        Error::transport()
                            .message("socket write failed")
                            .wrap(err)
                            .build());
                }
                else
                    break;
            }

            auto n = r.unwrap();
            if (n > 0)
            {
                bytes += n;
                continue;
            }

            if (time::expired(ddl))
            {
                if (bytes > 0)
                    break;
                else
                    return IOResult::Err(
                        Error::transport()
                            .timeout()
                            .message("write timeout")
                            .build());
            }

            auto ddl_ms =
                std::chrono::duration_cast<
                    std::chrono::milliseconds>(
                    time::elapsed(
                        time::monotonic_now(), ddl))
                    .count();
            auto wr = wait_writable(sock, ddl_ms);
            if (wr.is_err())
                return IOResult::Err(wr.unwrap_err());
        }

        return IOResult::Ok(bytes);
    }

    ReadAwaitable
    IOContext::async_read(
        net::BaseSocket &sock,
        util::ByteBuffer &buf)
    {
        return ReadAwaitable(*this, sock.view(), buf);
    }

    util::ResultV<void>
    IOContext::wait_readable(
        net::BaseSocket &sock,
        int timeout_ms)
    {
        using Ret = util::ResultV<void>;
        using util::Error;

        auto add_res = m_poller.add(
            sock.view(),
            EPOLLIN | EPOLLONESHOT);
        if (add_res.is_err())
        {
            return Ret::Err(
                Error::system()
                    .message("epoll add fd failed")
                    .wrap(add_res.unwrap_err())
                    .build());
        }

        auto evs = poller().wait(timeout_ms);
        if (evs.is_err())
        {
            return Ret::Err(
                Error::system()
                    .message("epoll wait fd failed")
                    .wrap(evs.unwrap_err())
                    .build());
        }
        return Ret::Ok();
    }

    ReadAwaitable
    IOContext::async_wait_readable(
        net::BaseSocket &sock,
        util::ByteBuffer &buf)
    {
        return ReadAwaitable(*this, sock.view(), buf);
    }

    util::ResultV<void>
    IOContext::wait_writable(
        net::BaseSocket &sock,
        int timeout_ms)
    {
        using Ret = util::ResultV<void>;
        using util::Error;

        auto add_res = m_poller.add(
            sock.view(),
            EPOLLOUT | EPOLLONESHOT);

        if (add_res.is_err())
        {
            return Ret::Err(
                Error::system()
                    .message("epoll add fd failed")
                    .wrap(add_res.unwrap_err())
                    .build());
        }

        auto evs = poller().wait(timeout_ms);
        if (evs.is_err())
        {
            return Ret::Err(
                Error::system()
                    .message("epoll wait fd failed")
                    .wrap(evs.unwrap_err())
                    .build());
        }

        return Ret::Ok();
    }

    ReadAwaitable::ReadAwaitable(
        IOContext &ctx, fd::FdView fd,
        util::ByteBuffer &buf)
        : m_ctx(ctx), m_fd(fd),
          m_buf(buf),
          result(IOResult::Err(util::Error{})) {}

    bool ReadAwaitable::await_ready()
        const noexcept { return false; }

    void ReadAwaitable::await_suspend(
        std::coroutine_handle<> h)
    {
        auto add_res =
            m_ctx.poller().add(
                m_fd,
                EPOLLIN | EPOLLONESHOT);
        if (add_res.is_err())
        {
        }

        // 同步阻塞在这里
        auto evs_res = m_ctx.poller().wait(-1);
        if (evs_res.is_err())
        {
        }

        auto evs = evs_res.unwrap();

        // 一旦返回，说明 fd 就绪
        h.resume();
    }

    IOResult
    ReadAwaitable::await_resume()
    {
        auto writable = m_buf.writable_size();
        auto span = m_buf.prepare(writable);

        ssize_t n = ::read(m_fd.fd, span.data(), writable);
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

    void ReadAwaitable::set_result(
        IOResult r)
    {
        result = std::move(r);
    }

}