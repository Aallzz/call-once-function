#ifndef TST_ADF_H
#define TST_ADF_H

#include <gtest/gtest.h>
#include "call_once_function.h"
#include <string>
#include <vector>

std::string update_string(std::string& x) {
    x += 'x';
    return x;
}

std::string update_string_2(std::string& x) {
    x += "yyy";
    return x;
}

int update_f(int a, int b) {
    return a + b;
}


int fact(int n) {
    if (n == 0) return 1;
    return n * fact(n - 1);
}

TEST(constructor, default_constructor) {
    std::string x;
    call_once_function<int(int)> f;
    EXPECT_THROW(f(11), std::bad_function_call);
}

TEST(constructor, function_constructor) {
    std::string x;
    call_once_function<std::string(std::string&)> f(update_string);
    x = f(x);
    EXPECT_EQ(x, "x");
}

TEST(constructor, move_constructor) {
    std::string x;
    call_once_function<std::string(std::string&)> g(update_string);
    call_once_function<std::string(std::string&)> f(std::move(g));
    EXPECT_THROW(g(x), std::bad_function_call);
    f(x);
    EXPECT_EQ(x, "x");
}

TEST(swap, swap_) {
    call_once_function<std::string(std::string&)> g(update_string);
    call_once_function<std::string(std::string&)> f(update_string_2);
    f.swap(g);
    std::string x = "a";
    std::string y = "b";
    x = f(x);
    y = g(y);
    EXPECT_EQ(x, "ax");
    EXPECT_EQ(y, "byyy");
}

TEST(smth, recursive_function) {
    call_once_function<int(int)> f(fact);
    EXPECT_EQ(f(10), 3628800);
}

TEST(smth, simpl_lambda) {
    auto c = [](int a){return a;};
    call_once_function<int(int)> f(c);
    EXPECT_EQ(f(15), 15);
}

TEST(smth, function_in_function) {
    call_once_function<int(int)> f(fact);
    call_once_function<int(int)> g(fact);
    EXPECT_EQ(g(f(3)), 720);
}

#endif // TST_ADF_H
