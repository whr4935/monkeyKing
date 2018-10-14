#include <iostream>
#include <gtest/gtest.h>

struct Foo
{
    int print()
    {
        std::cout << "Foo print" << std::endl;
        return 0;
    }
};

TEST(FooTest, 1)
{
    EXPECT_EQ(1, 1);

    Foo f;
    EXPECT_EQ(f.print(), 0);
}

