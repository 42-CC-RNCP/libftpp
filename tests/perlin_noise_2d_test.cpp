#include "mathematics/perlin_noise_2D.hpp"
#include <array>
#include <cmath>
#include <gtest/gtest.h>
#include <utility>

TEST(PerlinNoise2DTest, SameSeedProducesDeterministicSamples)
{
    const PerlinNoise2D noise_a(20260301LL);
    const PerlinNoise2D noise_b(20260301LL);

    const std::array<std::pair<float, float>, 8> points = {
        std::pair<float, float>{0.125f, 0.875f},
        std::pair<float, float>{1.25f, 2.5f},
        std::pair<float, float>{-1.75f, 3.125f},
        std::pair<float, float>{10.75f, -4.5f},
        std::pair<float, float>{31.03125f, 8.96875f},
        std::pair<float, float>{-64.25f, -32.75f},
        std::pair<float, float>{123.456f, 654.321f},
        std::pair<float, float>{-999.875f, 333.625f}};

    for (const auto& [x, y] : points) {
        const float first = noise_a.sample(x, y);
        const float second = noise_a.sample(x, y);
        const float from_other_instance = noise_b.sample(x, y);

        EXPECT_EQ(first, second);
        EXPECT_EQ(first, from_other_instance);
    }
}

TEST(PerlinNoise2DTest, DifferentSeedsUsuallyProduceDifferentSamples)
{
    const PerlinNoise2D noise_a(7LL);
    const PerlinNoise2D noise_b(11LL);

    const std::array<std::pair<float, float>, 8> points = {
        std::pair<float, float>{0.125f, 0.875f},
        std::pair<float, float>{1.25f, 2.5f},
        std::pair<float, float>{-1.75f, 3.125f},
        std::pair<float, float>{10.75f, -4.5f},
        std::pair<float, float>{31.03125f, 8.96875f},
        std::pair<float, float>{-64.25f, -32.75f},
        std::pair<float, float>{123.456f, 654.321f},
        std::pair<float, float>{-999.875f, 333.625f}};

    bool found_difference = false;
    for (const auto& [x, y] : points) {
        if (noise_a.sample(x, y) != noise_b.sample(x, y)) {
            found_difference = true;
            break;
        }
    }

    EXPECT_TRUE(found_difference);
}

TEST(PerlinNoise2DTest, IntegerLatticeSamplesAreZero)
{
    const PerlinNoise2D noise(42LL);

    for (int y = -8; y <= 8; ++y) {
        for (int x = -8; x <= 8; ++x) {
            const float value =
                noise.sample(static_cast<float>(x), static_cast<float>(y));
            EXPECT_FLOAT_EQ(value, 0.0f);
        }
    }
}

TEST(PerlinNoise2DTest, SamplesAreFiniteOnRepresentativePoints)
{
    const PerlinNoise2D noise(12345LL);

    const std::array<std::pair<float, float>, 10> points = {
        std::pair<float, float>{0.1f, 0.2f},
        std::pair<float, float>{0.9f, 0.7f},
        std::pair<float, float>{1.1f, 2.2f},
        std::pair<float, float>{2.3f, 4.7f},
        std::pair<float, float>{-0.3f, 1.4f},
        std::pair<float, float>{-3.6f, -2.8f},
        std::pair<float, float>{8.125f, -13.25f},
        std::pair<float, float>{64.75f, 32.5f},
        std::pair<float, float>{-128.875f, 256.625f},
        std::pair<float, float>{1024.125f, -2048.875f}};

    for (const auto& [x, y] : points) {
        const float value = noise.sample(x, y);
        EXPECT_TRUE(std::isfinite(value));
    }
}
