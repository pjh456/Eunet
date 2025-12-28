#include "eunet/net/sync_tcp_client.hpp"

namespace net::tcp
{
    SyncTCPClient::SyncTCPClient(core::Orchestrator &orch, Options o)
        : orch(orch), opts(o)
    {
    }

    SyncTCPClient::~SyncTCPClient() noexcept
    {
        close();
    }

    util::ResultV<void>
    SyncTCPClient::emit_event(const core::Event &event)
    {
        return orch.emit(event);
    }

    util::ResultV<void>
    SyncTCPClient::resolve_host(
        const std::string &host,
        sockaddr_in &out_addr)
    {
        auto resolve_host_res =
            emit_event(core::Event::info(
                core::EventType::DNS_RESOLVE_START,
                "Resolving host: " + host));

        addrinfo hints{};
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;

        addrinfo *res = nullptr;
        int err = ::getaddrinfo(host.c_str(), nullptr, &hints, &res);
        if (err != 0)
        {
            auto dns_failed_res =
                emit_event(core::Event::failure(
                    core::EventType::DNS_RESOLVE_START,
                    util::Error::from_gai(err, "DNS lookup failed")));
            return util::ResultV<void>::Err(
                util::Error::from_errno(err));
        }

        if (!res)
        {
            auto dns_empty_res =
                emit_event(core::Event::failure(
                    core::EventType::DNS_RESOLVE_DONE,
                    util::Error::internal("DNS lookup returned empty result")));
            return util::ResultV<void>::Err(util::Error::from_errno(EAI_NONAME));
        }

        out_addr.sin_family = AF_INET;
        out_addr.sin_port = 0;
        out_addr.sin_addr = ((sockaddr_in *)res->ai_addr)->sin_addr;

        ::freeaddrinfo(res);
        auto dns_done_res = emit_event(core::Event::info(
            core::EventType::DNS_RESOLVE_DONE,
            "Host resolved"));
        return util::ResultV<void>::Ok();
    }

    util::ResultV<void> SyncTCPClient::connect(const std::string &host, uint16_t port)
    {
        sockaddr_in addr{};
        auto res = resolve_host(host, addr);
        if (res.is_err())
            return util::ResultV<void>::Err(res.unwrap_err());

        addr.sin_port = htons(port);

        auto sock_res = platform::fd::Fd::socket(AF_INET, SOCK_STREAM, 0);
        if (sock_res.is_err())
        {
            auto socket_creation_failed_res =
                emit_event(core::Event::failure(
                    core::EventType::DNS_RESOLVE_DONE,
                    util::Error::internal("Socket creation failed")));

            return util::ResultV<void>::Err(sock_res.unwrap_err());
        }

        sock = std::move(sock_res.unwrap());

        auto connect_host_res =
            emit_event(core::Event::info(
                core::EventType::TCP_CONNECT_START,
                "Connecting to " + host + ":" + std::to_string(port)));
        if (connect_host_res.is_err())
            return util::ResultV<void>::Err(connect_host_res.unwrap_err());

        if (::connect(sock.get(), (struct sockaddr *)&addr, sizeof(addr)) < 0)
        {
            auto connect_failed_res =
                emit_event(core::Event::failure(
                    core::EventType::TCP_CONNECT_START,
                    util::Error::from_errno(errno, "Connect failed")));
            return util::ResultV<void>::Err(util::Error::from_errno(errno));
        }

        auto connect_success_res =
            emit_event(core::Event::info(
                core::EventType::TCP_CONNECT_SUCCESS,
                "Connection established"));
        return util::ResultV<void>::Ok();
    }

    util::ResultV<size_t>
    SyncTCPClient::send(const std::vector<std::byte> &data)
    {
        if (!sock)
            return util::ResultV<size_t>::Err(util::Error::from_errno(EBADF));

        auto http_send_res =
            emit_event(core::Event::info(
                core::EventType::HTTP_SENT,
                "Sending " + std::to_string(data.size()) + " bytes"));
        if (http_send_res.is_err())
            return util::ResultV<size_t>::Err(http_send_res.unwrap_err());

        ssize_t n = ::send(sock.get(), data.data(), data.size(), 0);

        if (n < 0)
        {
            auto send_failed_res = emit_event(core::Event::failure(
                core::EventType::HTTP_SENT,
                util::Error::from_errno(errno, "Send failed")));
            return util::ResultV<size_t>::Err(util::Error::from_errno(errno));
        }

        return util::ResultV<size_t>::Ok(static_cast<size_t>(n));
    }

    util::ResultV<size_t> SyncTCPClient::recv(std::vector<std::byte> &buffer, size_t size)
    {
        if (!sock)
            return util::ResultV<size_t>::Err(util::Error::from_errno(EBADF));

        buffer.resize(size);
        auto http_receive_res =
            emit_event(core::Event::info(
                core::EventType::HTTP_RECEIVED,
                "Receiving up to " + std::to_string(size) + " bytes"));
        if (http_receive_res.is_err())
            return util::ResultV<size_t>::Err(http_receive_res.unwrap_err());

        ssize_t n = ::recv(sock.get(), buffer.data(), size, 0);
        if (n < 0)
        {
            auto recv_failed_res =
                emit_event(core::Event::failure(
                    core::EventType::HTTP_RECEIVED,
                    util::Error::from_errno(errno, "Recv failed")));
            return util::ResultV<size_t>::Err(util::Error::from_errno(errno));
        }

        buffer.resize(static_cast<size_t>(n));
        return util::ResultV<size_t>::Ok(static_cast<size_t>(n));
    }

    void SyncTCPClient::close() noexcept
    {
        if (sock)
        {
            auto close_socket_res =
                emit_event(core::Event::info(
                    core::EventType::CONNECTION_CLOSED,
                    "Closing socket"));
            sock.reset(-1);
        }
    }
}
