#include <iostream>
#include <string>
#include "eunet/platform/time.hpp"

template <typename T>
    requires requires(T a) {
        a + a;
    }
T add(T a, T b)
{
    return a + b;
}

int main()
{
    std::cout
        << "Hello world from "
        << add(1, 1)
        << " and "
        << add(
               std::string("gcc, "),
               std::string("g++"))
        << std::endl;
}