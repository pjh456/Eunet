#include <cassert>
#include <iostream>
#include <vector>
#include <cstring>
#include <stdexcept>

#include "eunet/util/byte_buffer.hpp"

using util::ByteBuffer;

static std::vector<std::byte> make_bytes(const char *s)
{
    size_t n = std::strlen(s);
    std::vector<std::byte> v(n);
    std::memcpy(v.data(), s, n);
    return v;
}

static std::string to_string(std::span<const std::byte> s)
{
    return std::string(reinterpret_cast<const char *>(s.data()), s.size());
}

void test_basic_state()
{
    ByteBuffer buf(16);
    assert(buf.size() == 0);
    assert(buf.capacity() == 16);
    assert(buf.writable_size() == 16);
    assert(buf.empty());
}

void test_append_and_read()
{
    ByteBuffer buf(8);
    auto data = make_bytes("hello");
    buf.append(data);

    assert(buf.size() == 5);
    assert(!buf.empty());
    assert(to_string(buf.readable()) == "hello");
}

void test_prepare_commit()
{
    ByteBuffer buf(8);

    auto w = buf.prepare(4);
    std::memcpy(w.data(), "ABCD", 4);
    buf.commit(4);

    assert(buf.size() == 4);
    assert(to_string(buf.readable()) == "ABCD");
}

void test_consume_and_compact()
{
    ByteBuffer buf(8);
    buf.append(make_bytes("hello"));
    buf.consume(2); // consume "he"

    assert(buf.size() == 3);
    assert(to_string(buf.readable()) == "llo");

    // consume triggers compact internally
    buf.append(make_bytes("!!"));
    assert(to_string(buf.readable()) == "llo!!");
}

void test_compact_then_prepare()
{
    ByteBuffer buf(8);
    buf.append(make_bytes("12345"));
    buf.consume(3); // left "45"

    auto w = buf.prepare(3);
    std::memcpy(w.data(), "678", 3);
    buf.commit(3);

    assert(to_string(buf.readable()) == "45678");
}

void test_auto_grow()
{
    ByteBuffer buf(4);
    buf.append(make_bytes("abcd"));
    buf.append(make_bytes("efgh")); // must grow

    assert(buf.capacity() >= 8);
    assert(to_string(buf.readable()) == "abcdefgh");
}

void test_clear_and_reset()
{
    ByteBuffer buf(8);
    buf.append(make_bytes("hello"));

    buf.clear();
    assert(buf.size() == 0);
    assert(buf.capacity() == 8);

    buf.append(make_bytes("hi"));
    buf.reset();
    assert(buf.size() == 0);
    assert(buf.capacity() == 0);
}

void test_exceptions()
{
    ByteBuffer buf(8);
    bool thrown = false;

    try
    {
        buf.consume(1);
    }
    catch (const std::out_of_range &)
    {
        thrown = true;
    }
    assert(thrown);

    thrown = false;
    try
    {
        buf.commit(1);
    }
    catch (const std::logic_error &)
    {
        thrown = true;
    }
    assert(thrown);
}

void test_socket_like_flow()
{
    ByteBuffer buf(8);

    // recv #1
    {
        auto w = buf.prepare(5);
        std::memcpy(w.data(), "hello", 5);
        buf.commit(5);
    }

    // recv #2
    {
        auto w = buf.prepare(5);
        std::memcpy(w.data(), "world", 5);
        buf.commit(5);
    }

    assert(to_string(buf.readable()) == "helloworld");

    // application consumes part
    buf.consume(5);
    assert(to_string(buf.readable()) == "world");

    // recv #3 after consume
    {
        auto w = buf.prepare(3);
        std::memcpy(w.data(), "!!!", 3);
        buf.commit(3);
    }

    assert(to_string(buf.readable()) == "world!!!");
}

int main()
{
    std::cout << "Running ByteBuffer tests...\n";

    test_basic_state();
    test_append_and_read();
    test_prepare_commit();
    test_consume_and_compact();
    test_compact_then_prepare();
    test_auto_grow();
    test_clear_and_reset();
    test_exceptions();
    test_socket_like_flow();

    std::cout << "All ByteBuffer tests passed.\n";
    return 0;
}
