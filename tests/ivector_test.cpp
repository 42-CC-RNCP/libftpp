#include "mathematics/ivector2.hpp"
#include "mathematics/ivector3.hpp"
#include <gtest/gtest.h>

TEST(IVector2Test, DefaultConstructorInitializesToZero)
{
    IVector2<int> value;
    EXPECT_EQ(value.x, 0);
    EXPECT_EQ(value.y, 0);
}

TEST(IVector2Test, BasicArithmeticOperators)
{
    IVector2<int> left(6, 8);
    IVector2<int> right(2, 4);

    EXPECT_EQ(left + right, IVector2<int>(8, 12));
    EXPECT_EQ(left - right, IVector2<int>(4, 4));
    EXPECT_EQ(left * 2, IVector2<int>(12, 16));
    EXPECT_EQ(left / 2, IVector2<int>(3, 4));
}

TEST(IVector2Test, EqualityAndInequality)
{
    IVector2<int> first(1, 2);
    IVector2<int> same(1, 2);
    IVector2<int> different(1, 3);

    EXPECT_TRUE(first == same);
    EXPECT_FALSE(first != same);
    EXPECT_TRUE(first != different);
}

TEST(IVector2Test, LengthNormalizedDotAndCross)
{
    IVector2<float> value(3.0f, 4.0f);

    EXPECT_FLOAT_EQ(value.length(), 5.0f);

    IVector2<float> normalized = value.normalized();
    EXPECT_FLOAT_EQ(normalized.x, 0.6f);
    EXPECT_FLOAT_EQ(normalized.y, 0.8f);

    IVector2<float> other(2.0f, 1.0f);
    EXPECT_FLOAT_EQ(value.dot(other), 10.0f);

    IVector2<float> cross = IVector2<float>::cross(value, other);
    EXPECT_FLOAT_EQ(cross.x, 5.0f);
    EXPECT_FLOAT_EQ(cross.y, -5.0f);
}

TEST(IVector2Test, NormalizedZeroVectorReturnsZero)
{
    IVector2<float> zero;
    IVector2<float> normalized = zero.normalized();
    EXPECT_FLOAT_EQ(normalized.x, 0.0f);
    EXPECT_FLOAT_EQ(normalized.y, 0.0f);
}

TEST(IVector3Test, DefaultConstructorInitializesToZero)
{
    IVector3<int> value;
    EXPECT_EQ(value.x, 0);
    EXPECT_EQ(value.y, 0);
    EXPECT_EQ(value.z, 0);
}

TEST(IVector3Test, BasicArithmeticOperators)
{
    IVector3<int> left(9, 6, 3);
    IVector3<int> right(3, 2, 1);

    EXPECT_EQ(left + right, IVector3<int>(12, 8, 4));
    EXPECT_EQ(left - right, IVector3<int>(6, 4, 2));
    EXPECT_EQ(left * 2, IVector3<int>(18, 12, 6));
    EXPECT_EQ(left / 3, IVector3<int>(3, 2, 1));
}

TEST(IVector3Test, EqualityAndInequality)
{
    IVector3<int> first(7, 8, 9);
    IVector3<int> same(7, 8, 9);
    IVector3<int> different(7, 8, 10);

    EXPECT_TRUE(first == same);
    EXPECT_FALSE(first != same);
    EXPECT_TRUE(first != different);
}

TEST(IVector3Test, LengthAndNormalized)
{
    IVector3<float> value(1.0f, 2.0f, 2.0f);
    EXPECT_FLOAT_EQ(value.length(), 3.0f);

    IVector3<float> normalized = value.normalized();
    EXPECT_FLOAT_EQ(normalized.x, 1.0f / 3.0f);
    EXPECT_FLOAT_EQ(normalized.y, 2.0f / 3.0f);
    EXPECT_FLOAT_EQ(normalized.z, 2.0f / 3.0f);
}

TEST(IVector3Test, NormalizedZeroVectorReturnsZero)
{
    IVector3<float> zero;
    IVector3<float> normalized = zero.normalized();
    EXPECT_FLOAT_EQ(normalized.x, 0.0f);
    EXPECT_FLOAT_EQ(normalized.y, 0.0f);
    EXPECT_FLOAT_EQ(normalized.z, 0.0f);
}

TEST(IVector3Test, CrossProduct)
{
    IVector3<float> x_axis(1.0f, 0.0f, 0.0f);
    IVector3<float> y_axis(0.0f, 1.0f, 0.0f);

    IVector3<float> cross = x_axis.cross(y_axis);

    EXPECT_FLOAT_EQ(cross.x, 0.0f);
    EXPECT_FLOAT_EQ(cross.y, 0.0f);
    EXPECT_FLOAT_EQ(cross.z, 1.0f);
}
