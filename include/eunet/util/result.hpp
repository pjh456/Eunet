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

    /**
     * @brief 结果封装类 (Monad)
     *
     * 提供类似 Rust 的 Result<T, E> 机制，强制调用者处理错误。
     * 它要么包含一个成功的值 T，要么包含一个错误 E。
     *
     * @tparam T 成功时的数据类型
     * @tparam E 失败时的错误类型
     *
     * @note 禁止忽略该函数的返回值 ([[nodiscard]])
     * @note 禁止 T 与 E 相同
     */
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
        /**
         * @brief 如果是 Ok，则解包获取值；如果是 Err，则抛出异常
         *
         * @throw result_helper::bad_result_access 如果当前是 Err 状态
         * @return T& 内部值的引用
         */
        [[nodiscard]] T &unwrap() &
        {
            if (!is_ok())
                throw result_helper::
                    bad_result_access(
                        "Result::unwrap() called on Err");
            return std::get<T>(data);
        }

        /**
         * @brief 如果是 Ok，则解包获取值；如果是 Err，则抛出异常
         *
         * @throw result_helper::bad_result_access 如果当前是 Err 状态
         * @return const T& 内部值的常量引用
         */
        [[nodiscard]] const T &unwrap() const &
        {
            if (!is_ok())
                throw result_helper::
                    bad_result_access(
                        "Result::unwrap() called on Err");
            return std::get<T>(data);
        }

        /**
         * @brief 如果是 Ok，则解包获取值；如果是 Err，则抛出异常
         *
         * @throw result_helper::bad_result_access 如果当前是 Err 状态
         * @return T&& 内部值的右值引用
         */
        [[nodiscard]] T unwrap() &&
        {
            if (!is_ok())
                throw result_helper::
                    bad_result_access("unwrap on Err");
            return std::move(std::get<T>(data));
        }

        /**
         * @brief 如果是 Ok，则解包获取值；如果是 Err，则使用默认值 val
         *
         * @param val 为 Err 的默认返回值
         * @return const T& 内部值的常量引用或默认值
         */
        [[nodiscard]] const T &unwrap_or(const T &val) const
        {
            return is_ok() ? std::get<T>(data) : val;
        }

        /**
         * @brief 如果是 Err，则解包获取值；如果是 Ok，则抛出异常
         *
         * @throw result_helper::bad_result_access 如果当前是 Ok 状态
         * @return E& 内部值的引用
         */
        [[nodiscard]] E &unwrap_err() &
        {
            if (!is_err())
                throw result_helper::
                    bad_result_access(
                        "Result::unwrap_err() called on Ok");
            return std::get<E>(data);
        }

        /**
         * @brief 如果是 Err，则解包获取值；如果是 Ok，则抛出异常
         *
         * @throw result_helper::bad_result_access 如果当前是 Ok 状态
         * @return const E& 内部值的常量引用
         */
        [[nodiscard]] const E &unwrap_err() const &
        {
            if (!is_err())
                throw result_helper::
                    bad_result_access(
                        "Result::unwrap_err() called on Ok");
            return std::get<E>(data);
        }

        /**
         * @brief 如果是 Err，则解包获取值；如果是 Ok，则抛出异常
         *
         * @throw result_helper::bad_result_access 如果当前是 Ok 状态
         * @return E&& 内部值的右值引用
         */
        [[nodiscard]] E unwrap_err() &&
        {
            if (!is_err())
                throw result_helper::
                    bad_result_access("unwrap_err on Ok");
            return std::move(std::get<E>(data));
        }

        /**
         * @brief 如果是 Err，则解包获取值；如果是 Ok，则使用默认值 err
         *
         * @param err 为 Ok 的默认返回值
         * @return const E& 内部值的常量引用或默认值
         */
        [[nodiscard]] const E &unwrap_err_or(const E &err) const
        {
            return is_err() ? std::get<E>(data) : err;
        }

    public:
        /**
         * @brief 转换成功值 (Map)
         *
         * 如果当前是 Ok(v)，则应用函数 f(v) 并返回 Result::Ok(f(v))。
         * 如果当前是 Err(e)，则直接返回 Result::Err(e)。
         *
         * @tparam F 转换函数类型
         * @param f 接受 T 返回 U 的可调用对象
         * @return Result<U, E> 新的结果类型
         */
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

        /**
         * @brief 转换报错值 (Map_Err)
         *
         * 如果当前是 Err(e)，则应用函数 f(e) 并返回 Result::Err(f(e))。
         * 如果当前是 Ok(v)，则直接返回 Result::Ok(v)。
         *
         * @tparam F 转换函数类型
         * @param f 接受 E 返回 G 的可调用对象
         * @return Result<T, G> 新的结果类型
         */
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

    public:
        /**
         * @brief 链式调用 (FlatMap / AndThen)
         *
         * 如果当前是 Ok(v)，则调用 f(v)，f 必须返回一个 Result 类型。
         * 用于串联多个可能失败的操作。
         *
         * @param f 接受 T 返回 Result<U, E> 的可调用对象
         */
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

    /**
     * @brief 结果封装类 (Monad)
     *
     * 提供类似 Rust 的 Result<T, E> 机制，强制调用者处理错误。
     * 它要么包含一个成功的值 T，要么包含一个错误 E。
     * 该版本为 T = void 的特化版本。
     *
     * @tparam void 成功时的数据类型
     * @tparam E 失败时的错误类型
     *
     * @note 禁止忽略该函数的返回值 ([[nodiscard]])
     * @note 禁止 void 与 E 相同
     */
    template <typename E>
        requires result_helper::ValidResultTypes<void, E> &&
                 result_helper::NotResult<E>
    class [[nodiscard]] Result<void, E>
    {
    private:
        std::variant<std::monostate, E> data;

    public:
        static Result Ok() noexcept { return Result(std::monostate{}); }

        template <typename G>
            requires std::constructible_from<E, G &&>
        static Result Err(G &&err) noexcept(
            std::is_nothrow_constructible_v<E, G &&>)
        {
            return Result(E(std::forward<G>(err)));
        }

    public:
        explicit Result(std::monostate) : data(std::monostate{}) {}
        explicit Result(const E &err) : data(err) {}

        explicit Result(E &&err) noexcept : data(std::move(err)) {}

        explicit Result(const Result &)
            requires std::copy_constructible<E>
        = default;

        Result &operator=(const Result &)
            requires(std::is_copy_assignable_v<E>)
        = default;

        explicit Result(Result &&) noexcept = default;
        Result &operator=(Result &&) noexcept = default;

    public:
        bool is_ok() const noexcept { return std::holds_alternative<std::monostate>(data); }
        bool is_err() const noexcept { return std::holds_alternative<E>(data); }

    public:
        /**
         * @brief 如果是 Ok，则解包；如果是 Err，则抛出异常
         *
         * @throw result_helper::bad_result_access 如果当前是 Err 状态
         */
        void unwrap() const
        {
            if (is_err())
                throw result_helper::bad_result_access(
                    "Result<void>::unwrap() called on Err");
        }

        /**
         * @brief 如果是 Err，则解包获取值；如果是 Ok，则抛出异常
         *
         * @throw result_helper::bad_result_access 如果当前是 Ok 状态
         * @return E& 内部值的引用
         */
        [[nodiscard]] E &unwrap_err() &
        {
            if (!is_err())
                throw result_helper::
                    bad_result_access(
                        "Result::unwrap_err() called on Ok");
            return std::get<E>(data);
        }

        /**
         * @brief 如果是 Err，则解包获取值；如果是 Ok，则抛出异常
         *
         * @throw result_helper::bad_result_access 如果当前是 Ok 状态
         * @return const E& 内部值的常量引用
         */
        [[nodiscard]] const E &unwrap_err() const &
        {
            if (!is_err())
                throw result_helper::
                    bad_result_access(
                        "Result::unwrap_err() called on Ok");
            return std::get<E>(data);
        }

        /**
         * @brief 如果是 Err，则解包获取值；如果是 Ok，则抛出异常
         *
         * @throw result_helper::bad_result_access 如果当前是 Ok 状态
         * @return E&& 内部值的右值引用
         */
        [[nodiscard]] E unwrap_err() &&
        {
            if (!is_err())
                throw result_helper::
                    bad_result_access("unwrap_err on Ok");
            return std::move(std::get<E>(data));
        }

        /**
         * @brief 如果是 Err，则解包获取值；如果是 Ok，则使用默认值 err
         *
         * @param err 为 Ok 的默认返回值
         * @return const E& 内部值的常量引用或默认值
         */
        [[nodiscard]] const E &unwrap_err_or(const E &err) const
        {
            return is_err() ? std::get<E>(data) : err;
        }

    public:
        /**
         * @brief 转换成功值 (Map)
         *
         * 如果当前是 Ok(void)，则应用函数 f(void) 并返回 Result::Ok(f(void))。
         * 如果当前是 Err(e)，则直接返回 Result::Err(e)。
         *
         * @tparam F 转换函数类型
         * @param f 接受 void 返回 U 的可调用对象
         * @return Result<U, E> 新的结果类型
         */
        template <typename F>
            requires std::invocable<F>
        auto map(F &&f) const
            -> Result<std::invoke_result_t<F>, E>
        {
            using U = std::invoke_result_t<F>;

            if (is_ok())
            {
                if constexpr (std::is_void_v<U>)
                {
                    std::invoke(f);
                    return Result<void, E>::Ok();
                }
                else
                {
                    return Result<U, E>::Ok(std::invoke(f));
                }
            }
            else
            {
                return Result<U, E>::Err(std::get<E>(data));
            }
        }

        /**
         * @brief 转换报错值 (Map_Err)
         *
         * 如果当前是 Err(e)，则应用函数 f(e) 并返回 Result::Err(f(e))。
         * 如果当前是 Ok(void)，则直接返回 Result::Ok(void)。
         *
         * @tparam F 转换函数类型
         * @param f 接受 E 返回 G 的可调用对象
         * @return Result<void, G> 新的结果类型
         */
        template <typename F>
            requires std::invocable<F, E>
        auto map_err(F &&f) const
            -> Result<void, std::invoke_result_t<F, E>>
            requires result_helper::
                NotResult<std::invoke_result_t<F, E>>
        {
            using E2 = std::invoke_result_t<F, E>;

            if (is_err())
                return Result<void, E2>::Err(
                    std::invoke(f, std::get<E>(data)));
            else
                return Result<void, E2>::Ok();
        }

    public:
        /**
         * @brief 链式调用 (FlatMap / AndThen)
         *
         * 如果当前是 Ok(v)，则调用 f(v)，f 必须返回一个 Result 类型。
         * 用于串联多个可能失败的操作。
         *
         * @param f 接受 T 返回 Result<U, E> 的可调用对象
         */
        template <typename F>
            requires result_helper::
                ResultType<std::invoke_result_t<F>>
            auto and_then(F &&f) const & -> std::invoke_result_t<F>
        {
            using Ret = std::invoke_result_t<F>;

            if (is_ok())
                return std::invoke(f);
            else
                return Ret::Err(std::get<E>(data));
        }

        template <typename F>
            requires result_helper::
                ResultType<std::invoke_result_t<F>>
            auto and_then(F &&f) && -> std::invoke_result_t<F>
        {
            using Ret = std::invoke_result_t<F>;
            if (is_ok())
                return std::invoke(f);
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