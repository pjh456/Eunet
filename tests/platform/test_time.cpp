#include <cassert>
#include <iostream>
#include "eunet/platform/time.hpp"

using namespace platform::time;

void test_time_api()
{
    std::cout << "[test] platform::time\n";

    /* 1. monotonic clock 不回退 */
    {
        auto t1 = monotonic_now();
        auto t2 = monotonic_now();
        assert(t2 >= t1);
    }

    /* 2. elapsed / since */
    {
        auto start = monotonic_now();
        sleep_for(std::chrono::milliseconds(5));
        auto end = monotonic_now();

        auto d1 = elapsed(start, end);
        auto d2 = since(start);

        assert(d1.count() > 0);
        assert(d2 >= d1);
    }

    /* 3. duration 精度 */
    {
        Duration d = std::chrono::milliseconds(3);
        assert(d.count() == 3'000'000);
    }

    /* 4. unix <-> wall */
    {
        auto w = wall_now();
        auto u = to_unix(w);
        auto r = from_unix(u);

        auto diff = r - w;
        auto sec = std::chrono::duration_cast<std::chrono::seconds>(diff).count();
        assert(sec >= -1 && sec <= 1);
    }

    /* 5. sleep_until / deadline */
    {
        auto dl = deadline_after(std::chrono::milliseconds(10));
        assert(!expired(dl));

        sleep_until(dl);
        assert(expired(dl));
    }

    /* 6. 临时对象 / 值语义 */
    {
        auto d = elapsed(monotonic_now(), monotonic_now());
        (void)d;
    }

    std::cout << "[test] OK\n";
}

int main()
{
    test_time_api();
}