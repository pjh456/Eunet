#include <cassert>
#include <vector>
#include <cstddef>
#include <iostream>
#include "eunet/net/io_buffer.hpp"

void test_io_buffer()
{
    using namespace net;

    IOBuffer buf(8); // 小容量测试扩容

    // 初始可读/可写
    assert(buf.readable_bytes() == 0);
    assert(buf.writable_bytes() == 8);

    // append 写入数据
    std::vector<std::byte> data = {std::byte(1), std::byte(2), std::byte(3)};
    buf.append(data.data(), data.size());
    assert(buf.readable_bytes() == 3);
    assert(buf.peek()[0] == std::byte(1));

    // retrieve 读取数据
    buf.retrieve(2);
    assert(buf.readable_bytes() == 1);
    assert(buf.peek()[0] == std::byte(3));

    // 写空间自动扩容
    std::vector<std::byte> big_data(20, std::byte(0xFF));
    buf.append(big_data.data(), big_data.size());
    assert(buf.readable_bytes() == 21);

    // begin_write + has_written
    buf.ensure_writable(10);
    std::byte *write_ptr = buf.begin_write();
    for (int i = 0; i < 10; ++i)
        write_ptr[i] = std::byte(i);
    buf.has_written(10);
    assert(buf.readable_bytes() == 31);

    std::cout << "IOBuffer tests passed.\n";
}

int main()
{
    test_io_buffer();
    return 0;
}
