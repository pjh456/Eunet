#include "eunet/net/sync_tcp_client.hpp"

namespace net::tcp
{
    SyncTCPClient::SyncTCPClient(core::Timeline &tl, Options o)
        : timeline(tl), opts(o)
    {
    }

    SyncTCPClient::~SyncTCPClient() noexcept
    {
        close();
    }

    void SyncTCPClient::emit_event(const core::Event &event)
    {
        (void)timeline.push(event);
    }

    util::Result<void, platform::SysError>
    SyncTCPClient::resolve_host(const std::string &host, sockaddr_in &out_addr)
    {
        emit_event(core::Event::info(
            core::EventType::DNS_START,
            "Resolving host: " + host));

        addrinfo hints{};
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;

        addrinfo *res = nullptr;
        int err = ::getaddrinfo(host.c_str(), nullptr, &hints, &res);
        if (err != 0)
        {
            emit_event(core::Event::failure(
                core::EventType::DNS_START,
                {"net",
                 "DNS lookup failed: " + std::string(gai_strerror(err))}));
            return util::Result<void, platform::SysError>::Err(
                platform::SysError::from_errno(err));
        }

        if (!res)
        {
            emit_event(core::Event::failure(
                core::EventType::DNS_DONE,
                {"net", "DNS lookup returned empty result"}));
            return util::Result<void, platform::SysError>::Err(platform::SysError::from_errno(EAI_NONAME));
        }

        out_addr.sin_family = AF_INET;
        out_addr.sin_port = 0;
        out_addr.sin_addr = ((sockaddr_in *)res->ai_addr)->sin_addr;

        ::freeaddrinfo(res);
        emit_event(core::Event::info(core::EventType::DNS_DONE, "Host resolved"));
        return util::Result<void, platform::SysError>::Ok();
    }

    util::Result<void, platform::SysError> SyncTCPClient::connect(const std::string &host, uint16_t port)
    {
        sockaddr_in addr{};
        auto res = resolve_host(host, addr);
        if (res.is_err())
            return util::Result<void, platform::SysError>::Err(res.unwrap_err());

        addr.sin_port = htons(port);

        auto sock_res = platform::fd::Fd::socket(AF_INET, SOCK_STREAM, 0);
        if (sock_res.is_err())
        {
            emit_event(core::Event::failure(
                core::EventType::DNS_DONE,
                {"net", "Socket creation failed"}));
            return util::Result<void, platform::SysError>::Err(sock_res.unwrap_err());
        }

        sock = std::move(sock_res.unwrap());
        emit_event(core::Event::info(
            core::EventType::TCP_CONNECT,
            "Connecting to " + host + ":" + std::to_string(port)));

        if (::connect(sock.get(), (struct sockaddr *)&addr, sizeof(addr)) < 0)
        {
            emit_event(core::Event::failure(
                core::EventType::TCP_CONNECT,
                {"net", "Connect failed: " + std::string(strerror(errno))}));
            return util::Result<void, platform::SysError>::Err(platform::SysError::from_errno(errno));
        }

        emit_event(core::Event::info(
            core::EventType::TCP_ESTABLISHED,
            "Connection established"));
        return util::Result<void, platform::SysError>::Ok();
    }

    util::Result<size_t, platform::SysError>
    SyncTCPClient::send(const std::vector<std::byte> &data)
    {
        if (!sock)
            return util::Result<size_t, platform::SysError>::Err(platform::SysError::from_errno(EBADF));

        emit_event(core::Event::info(
            core::EventType::REQUEST_SENT,
            "Sending " + std::to_string(data.size()) + " bytes"));
        ssize_t n = ::send(sock.get(), data.data(), data.size(), 0);
        if (n < 0)
        {
            emit_event(core::Event::failure(
                core::EventType::REQUEST_SENT,
                {"net", "Send failed: " + std::string(strerror(errno))}));
            return util::Result<size_t, platform::SysError>::Err(platform::SysError::from_errno(errno));
        }

        return util::Result<size_t, platform::SysError>::Ok(static_cast<size_t>(n));
    }

    util::Result<size_t, platform::SysError> SyncTCPClient::recv(std::vector<std::byte> &buffer, size_t size)
    {
        if (!sock)
            return util::Result<size_t, platform::SysError>::Err(platform::SysError::from_errno(EBADF));

        buffer.resize(size);
        emit_event(core::Event::info(
            core::EventType::REQUEST_RECEIVED,
            "Receiving up to " + std::to_string(size) + " bytes"));

        ssize_t n = ::recv(sock.get(), buffer.data(), size, 0);
        if (n < 0)
        {
            emit_event(core::Event::failure(
                core::EventType::REQUEST_RECEIVED,
                {"net", "Recv failed: " + std::string(strerror(errno))}));
            return util::Result<size_t, platform::SysError>::Err(platform::SysError::from_errno(errno));
        }

        buffer.resize(static_cast<size_t>(n));
        return util::Result<size_t, platform::SysError>::Ok(static_cast<size_t>(n));
    }

    void SyncTCPClient::close() noexcept
    {
        if (sock)
        {
            emit_event(core::Event::info(
                core::EventType::CLOSED,
                "Closing socket"));
            sock.reset(-1);
        }
    }
}
