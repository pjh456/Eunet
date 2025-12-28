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
        std::cout << "[monotonic]t1=" << to_string(t1) << std::endl
                  << "[monotonic]t2=" << to_string(t2) << std::endl;
        assert(t2 >= t1);
    }

    /* 2. elapsed / since */
    {
        auto start = monotonic_now();
        sleep_for(std::chrono::milliseconds(5));
        auto end = monotonic_now();
        std::cout << "[monotonic]start=" << to_string(start) << std::endl
                  << "[monotonic]end=" << to_string(end) << std::endl;

        auto d1 = elapsed(start, end);
        auto d2 = since(start);
        std::cout << "[duration]elapsed(start, end)=" << d1 << std::endl
                  << "[duration]since(start)=" << d2 << std::endl;

        assert(d1.count() > 0);
        assert(d2 >= d1);
    }

    /* 3. duration 精度 */
    {
        Duration d = std::chrono::seconds(3);
        std::cout << "[duration]3s=" << d << std::endl;
        assert(d.count() == 3000);
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
        std::cout << "[monotonic]deadline after 10ms=" << to_string(dl) << std::endl;
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