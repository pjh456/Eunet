#ifndef INCLUDE_EUNET_UTIL_RESULT
#define INCLUDE_EUNET_UTIL_RESULT

#include <utility>
#include <variant>
#include <type_traits>
#include <string>
#include <string_view>
#include <stdexcept>
#include <functional>

namespace util
{
    namespace result_helper
    {
        template <typename>
        struct result_traits;

        template <typename T>
        using result_value_t =
            typename result_traits<std::remove_cvref_t<T>>::value_type;

        template <typename T>
        using result_error_t =
            typename result_traits<std::remove_cvref_t<T>>::error_type;

        template <typename T>
        concept ResultType =
            requires {
                typename result_traits<std::remove_cvref_t<T>>::value_type;
                typename result_traits<std::remove_cvref_t<T>>::error_type;
            };

        template <typename T>
        concept NotResult = !ResultType<T>;

        template <typename T, typename E>
        concept ValidResultTypes =
            !std::same_as<std::remove_cvref_t<T>,
                          std::remove_cvref_t<E>>;

        class bad_result_access : public std::logic_error
        {
        public:
            explicit bad_result_access(
                const std::string &msg)
                : std::logic_error(msg) {}
        };
    }

    template <typename T, typename E>
        requires result_helper::ValidResultTypes<T, E> &&
                 result_helper::NotResult<T> &&
                 result_helper::NotResult<E>
    class [[nodiscard]] Result
    {
    private:
        std::variant<T, E> data;

    public:
        template <typename U>
            requires std::constructible_from<T, U &&>
        static Result Ok(U &&val) noexcept(
            std::is_nothrow_constructible_v<T, U &&>)
        {
            return Result(T(std::forward<U>(val)));
        }

        template <typename G>
            requires std::constructible_from<E, G &&>
        static Result Err(G &&err) noexcept(
            std::is_nothrow_constructible_v<E, G &&>)
        {
            return Result(E(std::forward<G>(err)));
        }

    public:
        explicit Result(const T &val) : data(val) {}
        explicit Result(const E &err) : data(err) {}

        explicit Result(T &&val) noexcept : data(std::move(val)) {}
        explicit Result(E &&err) noexcept : data(std::move(err)) {}

        explicit Result(const Result &)
            requires std::copy_constructible<T> &&
                         std::copy_constructible<E>
        = default;

        Result &operator=(const Result &)
            requires(std::is_copy_assignable_v<T>) &&
                        (std::is_copy_assignable_v<E>)
        = default;

        explicit Result(Result &&) noexcept = default;
        Result &operator=(Result &&) noexcept = default;

    public:
        bool is_ok() const noexcept { return std::holds_alternative<T>(data); }
        bool is_err() const noexcept { return std::holds_alternative<E>(data); }

    public:
        [[nodiscard]] T &unwrap() &
        {
            if (!is_ok())
                throw result_helper::
                    bad_result_access(
                        "Result::unwrap() called on Err");
            return std::get<T>(data);
        }

        [[nodiscard]] const T &unwrap() const &
        {
            if (!is_ok())
                throw result_helper::
                    bad_result_access(
                        "Result::unwrap() called on Err");
            return std::get<T>(data);
        }

        [[nodiscard]] T unwrap() &&
        {
            if (!is_ok())
                throw result_helper::
                    bad_result_access("unwrap on Err");
            return std::move(std::get<T>(data));
        }

        [[nodiscard]] const T &unwrap_or(const T &val) const
        {
            return is_ok() ? std::get<T>(data) : val;
        }

        [[nodiscard]] E &unwrap_err() &
        {
            if (!is_err())
                throw result_helper::
                    bad_result_access(
                        "Result::unwrap_err() called on Ok");
            return std::get<E>(data);
        }

        [[nodiscard]] const E &unwrap_err() const &
        {
            if (!is_err())
                throw result_helper::
                    bad_result_access(
                        "Result::unwrap_err() called on Ok");
            return std::get<E>(data);
        }

        [[nodiscard]] E unwrap_err() &&
        {
            if (!is_err())
                throw result_helper::
                    bad_result_access("unwrap_err on Ok");
            return std::move(std::get<E>(data));
        }

        [[nodiscard]] const E &unwrap_err_or(const E &err) const
        {
            return is_err() ? std::get<E>(data) : err;
        }

    public:
        template <typename F>
            requires std::invocable<F, T>
        [[nodiscard]] auto map(F &&f) const
            -> Result<std::invoke_result_t<F, T>, E>
            requires result_helper::
                         ValidResultTypes<
                             std::invoke_result_t<F, T>, E> &&
                     (!std::is_same_v<
                         std::invoke_result_t<F, T>,
                         E>)
        {
            using U = std::invoke_result_t<F, T>;

            if (is_ok())
                return Result<U, E>::Ok(
                    std::invoke(f, std::get<T>(data)));
            else
                return Result<U, E>::Err(std::get<E>(data));
        }

        template <typename F>
            requires std::invocable<F, T> &&
                     std::is_same_v<
                         std::invoke_result_t<F, T>,
                         E>
        [[nodiscard]] E map_or(F &&f) const
        {
            if (is_ok())
                return std::invoke(f, std::get<T>(data));
            else
                return std::get<E>(data);
        }

        template <typename F>
            requires std::invocable<F, E>
        [[nodiscard]] auto map_err(F &&f) const
            -> Result<T, std::invoke_result_t<F, E>>
            requires result_helper::
                         ValidResultTypes<
                             T, std::invoke_result_t<F, E>> &&
                     (!std::is_same_v<
                         std::invoke_result_t<F, E>,
                         T>)
        {
            using E2 = std::invoke_result_t<F, E>;

            if (is_err())
                return Result<T, E2>::Err(
                    std::invoke(f, std::get<E>(data)));
            else
                return Result<T, E2>::Ok(std::get<T>(data));
        }

        template <typename F>
            requires std::invocable<F, E> &&
                     std::is_same_v<
                         std::invoke_result_t<F, E>,
                         T>
        [[nodiscard]] T map_err_or(F &&f) const
        {
            if (is_err())
                return std::invoke(f, std::get<E>(data));
            else
                return std::get<T>(data);
        }

    public:
        template <typename F>
        auto and_then(F &&f)
            const & -> std::invoke_result_t<F, const T &>
        {
            using Ret = std::invoke_result_t<F, const T &>;
            if (is_ok())
                return std::invoke(f, std::get<T>(data));
            else
                return Ret::Err(std::get<E>(data));
        }

        template <typename F>
        auto and_then(F &&f)
            && -> std::invoke_result_t<F, T>
        {
            using Ret = std::invoke_result_t<F, T>;
            if (is_ok())
                return std::invoke(f, std::move(std::get<T>(data)));
            else
                return Ret::Err(std::move(std::get<E>(data)));
        }
    };

    namespace result_helper
    {
        template <typename T, typename E>
        struct result_traits<util::Result<T, E>>
        {
            using value_type = T;
            using error_type = E;
        };
    }
}

#endif // INCLUDE_EUNET_UTIL_RESULT