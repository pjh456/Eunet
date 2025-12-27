#include <iostream>
#include <cstdlib>

#define TEST_ASSERT(expr)                                          \
    do                                                             \
    {                                                              \
        if (!(expr))                                               \
        {                                                          \
            std::cerr << "[FAIL] " << __FILE__ << ":" << __LINE__  \
                      << " assertion failed: " #expr << std::endl; \
            std::exit(1);                                          \
        }                                                          \
    } while (0)

#define TEST_EXPECT(expr)                                            \
    do                                                               \
    {                                                                \
        if (!(expr))                                                 \
        {                                                            \
            std::cerr << "[WARN] " << __FILE__ << ":" << __LINE__    \
                      << " expectation failed: " #expr << std::endl; \
        }                                                            \
    } while (0)

#include "eunet/platform/capability.hpp"

using namespace platform::capability;

void test_cap_mapping()
{
    TEST_ASSERT(helper::to_linux_cap(Capability::RawSocket) == CAP_NET_RAW);
    TEST_ASSERT(helper::to_linux_cap(Capability::BindPrivilegedPort) == CAP_NET_BIND_SERVICE);

    TEST_ASSERT(helper::linux_to_cap(CAP_NET_RAW) == Capability::RawSocket);
    TEST_ASSERT(helper::linux_to_cap(CAP_NET_BIND_SERVICE) == Capability::BindPrivilegedPort);
}

void test_has_permitted()
{
    auto res = helper::has_permitted(CAP_NET_RAW);

    if (res.is_ok())
    {
        // 返回 true / false 都是合法的
        TEST_EXPECT(res.unwrap() == true || res.unwrap() == false);
    }
    else
    {
        const auto &err = res.unwrap_err();
        TEST_ASSERT(err.code == CapabilityErrorCode::GetProcCapsFailed ||
                    err.code == CapabilityErrorCode::GetFlagFailed);
    }
}

void test_state()
{
    auto &mgr = CapabilityManager::instance();

    auto res = mgr.state(Capability::RawSocket);
    if (res.is_ok())
    {
        TEST_ASSERT(res.unwrap() == CapabilityState::Available ||
                    res.unwrap() == CapabilityState::Missing);
    }
    else
    {
        TEST_ASSERT(res.unwrap_err().code == CapabilityErrorCode::GetProcCapsFailed ||
                    res.unwrap_err().code == CapabilityErrorCode::GetFlagFailed);
    }
}

void test_enable_disable()
{
    auto &mgr = CapabilityManager::instance();

    auto state = mgr.state(Capability::RawSocket);
    if (state.is_err())
        return; // 系统异常，跳过

    if (state.unwrap() == CapabilityState::Missing)
    {
        auto res = mgr.enable(Capability::RawSocket);
        TEST_ASSERT(res.is_err());
        TEST_ASSERT(res.unwrap_err().code == CapabilityErrorCode::NotPermitted);
        return;
    }

    // 有权限，尝试 enable / disable
    {
        auto res = mgr.enable(Capability::RawSocket);
        TEST_EXPECT(res.is_ok());
    }

    {
        auto res = mgr.disable(Capability::RawSocket);
        TEST_EXPECT(res.is_ok());
    }
}

void test_drop_all_effective()
{
    auto &mgr = CapabilityManager::instance();

    auto res = mgr.drop_all_effective();
    if (res.is_ok())
    {
        res.unwrap();
    }
    else
    {
        TEST_ASSERT(res.unwrap_err().code == CapabilityErrorCode::GetProcCapsFailed ||
                    res.unwrap_err().code == CapabilityErrorCode::SetFlagFailed ||
                    res.unwrap_err().code == CapabilityErrorCode::SetProcFailed);
    }
}

void test_scoped_capability()
{
    auto res = ScopedCapability::acquire(Capability::RawSocket);
    if (res.is_ok())
    {
        // 析构时应自动 disable（best effort）
        ScopedCapability guard = std::move(res.unwrap());
    }
    else
    {
        TEST_EXPECT(res.unwrap_err().code == CapabilityErrorCode::NotPermitted);
    }
}

int main()
{
    std::cout << "[TEST] Capability tests start\n";

    test_cap_mapping();
    test_has_permitted();
    test_state();
    test_enable_disable();
    test_drop_all_effective();
    test_scoped_capability();

    std::cout << "[TEST] Capability tests finished\n";
    return 0;
}
