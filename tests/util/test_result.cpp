#include <cassert>
#include <string>
#include <type_traits>
#include <iostream>

#include "eunet/util/result.hpp"

using util::Result;
using util::result_helper::bad_result_access;
using util::result_helper::NotResult;
using util::result_helper::result_error_t;
using util::result_helper::result_value_t;
using util::result_helper::ResultType;
using util::result_helper::ValidResultTypes;

/* ============================================================
 * 编译期测试：traits / concepts
 * ============================================================ */

static_assert(ResultType<Result<int, std::string>>);
static_assert(ResultType<const Result<int, std::string> &>);
static_assert(!ResultType<int>);
static_assert(NotResult<int>);

static_assert(std::same_as<
              result_value_t<Result<int, std::string>>,
              int>);

static_assert(std::same_as<
              result_error_t<Result<int, std::string>>,
              std::string>);

static_assert(ValidResultTypes<int, std::string>);
static_assert(!ValidResultTypes<int, int>);

/* ============================================================
 * 编译期测试：map
 * ============================================================ */

static_assert(requires {
    Result<int, std::string>{1}.map(
        [](int x)
        { return x + 1; });
});

static_assert(!requires {
    Result<int, std::string>{1}.map(
        [](int) -> std::string
        { return "x"; });
});

/* ============================================================
 * 编译期测试：map_err
 * ============================================================ */

static_assert(requires {
    Result<int, std::string>{1}.map_err(
        [](const std::string &)
        { return long{1}; });
});

static_assert(!requires {
    Result<int, std::string>{1}.map_err(
        [](const std::string &) -> int
        { return 1; });
});

/* ============================================================
 * 编译期测试：and_then
 * ============================================================ */

static_assert(requires {
    Result<int, std::string>{1}.and_then(
        [](int x)
        {
            return Result<int, std::string>::Ok(x + 1);
        });
});

static_assert(!requires {
    Result<int, std::string>{1}.and_then(
        [](int)
        { return 42; });
});

static_assert(!requires {
    Result<int, std::string>{1}.and_then(
        [](int)
        {
            return Result<int, int *>::Ok(1);
        });
});

/* ============================================================
 * 编译期测试：Result<void, E>
 * ============================================================ */

static_assert(ResultType<Result<void, std::string>>);
static_assert(std::same_as<
              result_value_t<Result<void, std::string>>,
              void>);

static_assert(std::same_as<
              result_error_t<Result<void, std::string>>,
              std::string>);

// map: void -> U
static_assert(requires {
    Result<void, std::string>::Ok().map(
        []
        { return 1; });
});

// map: void -> void
static_assert(requires {
    Result<void, std::string>::Ok().map(
        [] {});
});

// map_err
static_assert(requires {
    Result<void, std::string>::Err("x").map_err(
        [](const std::string &)
        { return 42; });
});

// and_then
static_assert(requires {
    Result<void, std::string>::Ok().and_then(
        []
        {
            return Result<int, std::string>::Ok(1);
        });
});

/* ============================================================
 * 运行期测试：基础语义
 * ============================================================ */

void test_basic()
{
    auto ok = Result<int, std::string>::Ok(42);
    auto err = Result<int, std::string>::Err("fail");

    assert(ok.is_ok());
    assert(!ok.is_err());
    assert(err.is_err());

    assert(ok.unwrap() == 42);
    assert(err.unwrap_err() == "fail");
}

void test_unwrap_exceptions()
{
    auto ok = Result<int, std::string>::Ok(1);
    auto err = Result<int, std::string>::Err("e");

    bool threw = false;
    try
    {
        (void)err.unwrap();
    }
    catch (const bad_result_access &)
    {
        threw = true;
    }
    assert(threw);

    threw = false;
    try
    {
        (void)ok.unwrap_err();
    }
    catch (const bad_result_access &)
    {
        threw = true;
    }
    assert(threw);
}

void test_void_unwrap_exceptions()
{
    auto ok = Result<void, std::string>::Ok();
    auto err = Result<void, std::string>::Err("e");

    bool threw = false;
    try
    {
        err.unwrap();
    }
    catch (const bad_result_access &)
    {
        threw = true;
    }
    assert(threw);

    threw = false;
    try
    {
        (void)ok.unwrap_err();
    }
    catch (const bad_result_access &)
    {
        threw = true;
    }
    assert(threw);
}

void test_void_basic()
{
    auto ok = Result<void, std::string>::Ok();
    auto err = Result<void, std::string>::Err("fail");

    assert(ok.is_ok());
    assert(!ok.is_err());

    assert(err.is_err());
    assert(!err.is_ok());

    ok.unwrap();
    assert(err.unwrap_err() == "fail");
}

/* ============================================================
 * map 行为
 * ============================================================ */

void test_map()
{
    auto ok = Result<int, std::string>::Ok(2);
    auto err = Result<int, std::string>::Err("x");

    auto r1 = ok.map(
        [](int x)
        { return x * 3; });
    auto r2 = err.map(
        [](int x)
        { return x * 3; });

    assert(r1.unwrap() == 6);
    assert(r2.unwrap_err() == "x");
}

/* ============================================================
 * map_err 行为
 * ============================================================ */

void test_map_err()
{
    auto ok = Result<int, std::string>::Ok(5);
    auto err = Result<int, std::string>::Err("err");

    auto r1 = ok.map_err(
        [](const std::string &)
        { return long{1}; });

    auto r2 = err.map_err(
        [](const std::string &s)
        { return long(s.size()); });

    static_assert(
        std::same_as<
            decltype(r1),
            Result<int, long>>);

    assert(r1.unwrap() == 5);
    assert(r2.unwrap_err() == 3);
}

/* ============================================================
 * and_then 行为
 * ============================================================ */

void test_and_then()
{
    auto ok = Result<int, std::string>::Ok(1);
    auto err = Result<int, std::string>::Err("x");

    auto r1 = ok.and_then(
        [](int x)
        { return Result<int, std::string>::Ok(x + 1); });

    auto r2 = err.and_then(
        [](int x)
        { return Result<int, std::string>::Ok(x + 1); });

    assert(r1.unwrap() == 2);
    assert(r2.unwrap_err() == "x");
}

void test_void_map_value()
{
    auto ok = Result<void, std::string>::Ok();
    auto err = Result<void, std::string>::Err("x");

    auto r1 = ok.map(
        []
        { return 42; });

    auto r2 = err.map(
        []
        { return 42; });

    assert(r1.unwrap() == 42);
    assert(r2.unwrap_err() == "x");
}

void test_void_map_void()
{
    int counter = 0;

    auto ok = Result<void, std::string>::Ok();
    auto err = Result<void, std::string>::Err("x");

    (void)ok.map([&]
                 { ++counter; });
    (void)err.map([&]
                  { ++counter; });

    assert(counter == 1);
}

void test_void_map_err()
{
    auto ok = Result<void, std::string>::Ok();
    auto err = Result<void, std::string>::Err("abc");

    auto r1 = ok.map_err(
        [](const std::string &)
        { return 1; });

    auto r2 = err.map_err(
        [](const std::string &s)
        { return s.size(); });

    assert(r1.is_ok());
    assert(r2.unwrap_err() == 3);
}

void test_void_and_then()
{
    auto ok = Result<void, std::string>::Ok();
    auto err = Result<void, std::string>::Err("x");

    auto r1 = ok.and_then(
        []
        {
            return Result<int, std::string>::Ok(7);
        });

    auto r2 = err.and_then(
        []
        {
            return Result<int, std::string>::Ok(7);
        });

    assert(r1.unwrap() == 7);
    assert(r2.unwrap_err() == "x");
}

/* ============================================================
 * main
 * ============================================================ */

int main()
{
    test_basic();
    test_void_basic();

    test_unwrap_exceptions();
    test_void_unwrap_exceptions();

    test_map();
    test_map_err();
    test_and_then();

    test_void_map_value();
    test_void_map_void();
    test_void_map_err();
    test_void_and_then();

    std::cout << "[Result] all tests passed.\n";
    return 0;
}
