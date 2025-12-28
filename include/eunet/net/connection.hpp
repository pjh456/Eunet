#ifndef INCLUDE_EUNET_NET_CONNECTION
#define INCLUDE_EUNET_NET_CONNECTION

#include <span>
#include <cstddef>

#include "eunet/util/result.hpp"
#include "eunet/util/error.hpp"
#include "eunet/platform/fd.hpp"
#include "eunet/platform/time.hpp"

namespace net
{
    class Connection
    {
    public:
        virtual ~Connection() = default;

    public:
        virtual platform::fd::FdView fd() const noexcept = 0;
        virtual void close() noexcept = 0;
        virtual bool is_open() const noexcept = 0;

    public:
        virtual util::ResultV<size_t>
        read(std::byte *buf, size_t len,
             platform::time::Duration timeout) = 0;

        virtual util::ResultV<size_t>
        write(const std::byte *buf, size_t len,
              platform::time::Duration timeout) = 0;

    public:
        virtual bool has_pending_output() const noexcept { return false; }
        virtual util::ResultV<void> flush() { return util::ResultV<void>::Ok(); }
    };

}

#endif // INCLUDE_EUNET_NET_CONNECTION