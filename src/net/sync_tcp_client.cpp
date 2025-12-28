#include "eunet/net/sync_tcp_client.hpp"

namespace net::tcp
{
    util::ResultV<void>
    SyncTCPClient::emit_info(
        SyncTCPClient &client,
        core::EventType type,
        const std::string &msg)
    {
        auto res = client.emit_event(core::Event::info(type, msg));
        client.handle_telemetry(res);
        if (res.is_err())
            return util::ResultV<void>::Err(res.unwrap_err());
        return util::ResultV<void>::Ok();
    }

    util::ResultV<void>
    SyncTCPClient::emit_failure(
        SyncTCPClient &client,
        core::EventType type,
        const util::Error &err)
    {
        auto res = client.emit_event(core::Event::failure(type, err));
        client.handle_telemetry(res);
        if (res.is_err())
            return util::ResultV<void>::Err(res.unwrap_err());
        return util::ResultV<void>::Ok();
    }

    void SyncTCPClient::handle_telemetry(
        const util::ResultV<void> &emit_res) const
    {
        if (emit_res.is_err())
            fprintf(
                stderr,
                "[Telemetry Warning] %s\n",
                emit_res.unwrap_err().format().c_str());
    }

    SyncTCPClient::SyncTCPClient(core::Orchestrator &orch, Options o)
        : orch(orch), opts(o) {}

    SyncTCPClient::~SyncTCPClient() noexcept { close(); }

    util::ResultV<void>
    SyncTCPClient::emit_event(
        const core::Event &event) { return orch.emit(event); }

    util::ResultV<void>
    SyncTCPClient::resolve_host(
        const std::string &host,
        sockaddr_in &out_addr)
    {
        addrinfo hints{};
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;

        addrinfo *res = nullptr;
        int err = ::getaddrinfo(host.c_str(), nullptr, &hints, &res);
        if (err != 0)
            return util::ResultV<void>::Err(
                util::Error::from_gai(err, host));

        if (!res)
            return util::ResultV<void>::Err(
                util::Error::internal("getaddrinfo returned null"));

        out_addr.sin_family = AF_INET;
        out_addr.sin_addr = ((sockaddr_in *)res->ai_addr)->sin_addr;
        ::freeaddrinfo(res);

        return util::ResultV<void>::Ok();
    }

    util::ResultV<void> SyncTCPClient::connect(const std::string &host, uint16_t port)
    {
        sockaddr_in addr{};
        (void)emit_info(
            *this,
            core::EventType::DNS_RESOLVE_START,
            host);

        auto dns_res = resolve_host(host, addr);
        if (dns_res.is_err())
        {
            auto dns_err = dns_res.unwrap_err();
            (void)emit_failure(
                *this,
                core::EventType::DNS_RESOLVE_DONE,
                dns_err);
            return util::ResultV<void>::Err(dns_err);
        }

        addr.sin_port = htons(port);

        auto sock_res =
            platform::fd::Fd::socket(AF_INET, SOCK_STREAM, 0);
        if (sock_res.is_err())
        {
            auto sock_err = sock_res.unwrap_err();
            (void)emit_failure(
                *this,
                core::EventType::TCP_CONNECT_START,
                sock_err);
            return util::ResultV<void>::Err(sock_err);
        }

        sock = std::move(sock_res.unwrap());

        (void)emit_info(
            *this,
            core::EventType::TCP_CONNECT_START,
            host);

        if (::connect(sock.get(), (struct sockaddr *)&addr, sizeof(addr)) < 0)
        {
            auto connect_err = util::Error::from_errno(errno, "connect");
            (void)emit_failure(
                *this,
                core::EventType::TCP_CONNECT_START,
                connect_err);
            return util::ResultV<void>::Err(connect_err);
        }

        (void)emit_info(
            *this,
            core::EventType::TCP_CONNECT_SUCCESS,
            "connected");
        return util::ResultV<void>::Ok();
    }

    util::ResultV<size_t>
    SyncTCPClient::send(
        const std::vector<std::byte> &data)
    {
        if (!sock)
            return util::ResultV<size_t>::Err(
                util::Error::from_errno(EBADF));

        auto res = emit_info(
            *this,
            core::EventType::HTTP_SENT,
            "Sending " + std::to_string(data.size()) + " bytes");
        if (res.is_err())
            return util::ResultV<size_t>::Err(res.unwrap_err());

        ssize_t n = ::send(sock.get(), data.data(), data.size(), 0);
        if (n < 0)
        {
            auto err = util::Error::from_errno(errno, "Send failed");
            (void)emit_failure(*this, core::EventType::HTTP_SENT, err);
            return util::ResultV<size_t>::Err(err);
        }

        return util::ResultV<size_t>::Ok(static_cast<size_t>(n));
    }

    util::ResultV<size_t>
    SyncTCPClient::recv(
        std::vector<std::byte> &buffer,
        size_t size)
    {
        if (!sock)
            return util::ResultV<size_t>::Err(
                util::Error::from_errno(EBADF));

        buffer.resize(size);
        auto res = emit_info(
            *this,
            core::EventType::HTTP_RECEIVED,
            "Receiving up to " + std::to_string(size) + " bytes");
        if (res.is_err())
            return util::ResultV<size_t>::Err(res.unwrap_err());

        ssize_t n = ::recv(sock.get(), buffer.data(), size, 0);
        if (n < 0)
        {
            auto err = util::Error::from_errno(errno, "Recv failed");
            (void)emit_failure(*this, core::EventType::HTTP_RECEIVED, err);
            return util::ResultV<size_t>::Err(err);
        }

        buffer.resize(static_cast<size_t>(n));
        return util::ResultV<size_t>::Ok(static_cast<size_t>(n));
    }

    void SyncTCPClient::close() noexcept
    {
        if (sock)
        {
            (void)emit_info(
                *this,
                core::EventType::CONNECTION_CLOSED,
                "Closing socket");
            sock.reset(-1);
        }
    }
}
