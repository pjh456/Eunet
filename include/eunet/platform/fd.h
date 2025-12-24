#include <tuple>

namespace platform::fd
{
    class Fd
    {
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

    public:
        static Fd socket(
            int domain,
            int type,
            int protocol) noexcept;

        static std::tuple<Fd, Fd> pipe() noexcept;

    public:
        int release() noexcept;
        void reset(int new_fd) noexcept;
    };
}