#include "mathematics/random_2D_coordinate_generator.hpp"
#include <array>
#include <cmath>
#include <cstdint>
#include <gtest/gtest.h>
#include <limits>
#include <vector>

uint64_t to_u64(const long long value)
{
    return static_cast<uint64_t>(value);
}

double to_unit_interval(const uint64_t value)
{
    return static_cast<double>(value)
           / static_cast<double>(std::numeric_limits<uint64_t>::max());
}

double pearson_correlation(const std::vector<double>& a,
                           const std::vector<double>& b)
{
    if (a.size() != b.size() || a.empty()) {
        return 0.0;
    }

    long double sum_a = 0.0L;
    long double sum_b = 0.0L;
    long double sum_aa = 0.0L;
    long double sum_bb = 0.0L;
    long double sum_ab = 0.0L;

    for (size_t index = 0; index < a.size(); ++index) {
        const long double va = static_cast<long double>(a[index]);
        const long double vb = static_cast<long double>(b[index]);
        sum_a += va;
        sum_b += vb;
        sum_aa += va * va;
        sum_bb += vb * vb;
        sum_ab += va * vb;
    }

    const long double sample_count = static_cast<long double>(a.size());
    const long double numerator = (sample_count * sum_ab) - (sum_a * sum_b);
    const long double denom_left = (sample_count * sum_aa) - (sum_a * sum_a);
    const long double denom_right = (sample_count * sum_bb) - (sum_b * sum_b);
    const long double denominator = std::sqrt(denom_left * denom_right);

    if (denominator <= 0.0L) {
        return 0.0;
    }

    return static_cast<double>(numerator / denominator);
}

TEST(Random2DCoordinateGeneratorTest, DeterministicForSameInputAndSeed)
{
    Random2DCoordinateGenerator generator_a;
    Random2DCoordinateGenerator generator_b;

    EXPECT_EQ(generator_a.seed(), generator_b.seed());

    constexpr long long x = 123456789LL;
    constexpr long long y = -987654321LL;

    const long long first = generator_a(x, y);
    const long long second = generator_a(x, y);
    const long long third = generator_a(x, y);
    const long long from_other_instance = generator_b(x, y);

    EXPECT_EQ(first, second);
    EXPECT_EQ(second, third);
    EXPECT_EQ(first, from_other_instance);
}

TEST(Random2DCoordinateGeneratorTest, DifferentSeedsChangeOutput)
{
    Random2DCoordinateGenerator generator_a;
    Random2DCoordinateGenerator generator_b;

    generator_a.set_seed(7);
    generator_b.set_seed(11);

    const std::array<std::pair<long long, long long>, 8> coordinates = {
        std::pair<long long, long long>{0, 0},
        std::pair<long long, long long>{1, 0},
        std::pair<long long, long long>{0, 1},
        std::pair<long long, long long>{7, 13},
        std::pair<long long, long long>{42, -17},
        std::pair<long long, long long>{-100, 100},
        std::pair<long long, long long>{123456, 654321},
        std::pair<long long, long long>{-999999, 333333}};

    bool found_difference = false;
    for (const auto& [x, y] : coordinates) {
        if (generator_a(x, y) != generator_b(x, y)) {
            found_difference = true;
            break;
        }
    }

    EXPECT_TRUE(found_difference);
}

TEST(Random2DCoordinateGeneratorTest, ResettingSameSeedReproducesSameSequence)
{
    Random2DCoordinateGenerator generator;

    const std::array<std::pair<long long, long long>, 8> coordinates = {
        std::pair<long long, long long>{0, 0},
        std::pair<long long, long long>{1, 0},
        std::pair<long long, long long>{0, 1},
        std::pair<long long, long long>{7, 13},
        std::pair<long long, long long>{42, -17},
        std::pair<long long, long long>{-100, 100},
        std::pair<long long, long long>{123456, 654321},
        std::pair<long long, long long>{-999999, 333333}};

    generator.set_seed(20260301LL);
    std::array<long long, coordinates.size()> first_pass{};
    for (size_t i = 0; i < coordinates.size(); ++i) {
        first_pass[i] = generator(coordinates[i].first, coordinates[i].second);
    }

    generator.set_seed(20260301LL);
    for (size_t i = 0; i < coordinates.size(); ++i) {
        const long long second_pass =
            generator(coordinates[i].first, coordinates[i].second);
        EXPECT_EQ(first_pass[i], second_pass);
    }
}

TEST(Random2DCoordinateGeneratorTest, UniformityChiSquareAcrossBuckets)
{
    Random2DCoordinateGenerator generator;

    constexpr size_t bucket_count = 64;
    constexpr long long width = 256;
    constexpr long long height = 256;
    constexpr size_t sample_count = static_cast<size_t>(width * height);

    std::array<size_t, bucket_count> buckets{};
    for (long long y = 0; y < height; ++y) {
        for (long long x = 0; x < width; ++x) {
            const uint64_t value = to_u64(generator(x, y));
            const size_t bucket_index =
                static_cast<size_t>(value % bucket_count);
            ++buckets[bucket_index];
        }
    }

    const double expected =
        static_cast<double>(sample_count) / static_cast<double>(bucket_count);
    double chi_square = 0.0;
    for (const size_t observed : buckets) {
        const double diff = static_cast<double>(observed) - expected;
        chi_square += (diff * diff) / expected;
    }

    constexpr double kChiSquareUpperBound = 120.0;
    EXPECT_LT(chi_square, kChiSquareUpperBound);
}

TEST(Random2DCoordinateGeneratorTest, AdjacentCoordinatesHaveLowAutocorrelation)
{
    Random2DCoordinateGenerator generator;

    constexpr long long width = 384;
    constexpr long long height = 256;
    std::vector<double> current;
    std::vector<double> adjacent;
    current.reserve(static_cast<size_t>((width - 1) * height));
    adjacent.reserve(static_cast<size_t>((width - 1) * height));

    for (long long y = 0; y < height; ++y) {
        for (long long x = 0; x < width - 1; ++x) {
            current.push_back(to_unit_interval(to_u64(generator(x, y))));
            adjacent.push_back(to_unit_interval(to_u64(generator(x + 1, y))));
        }
    }

    const double correlation = pearson_correlation(current, adjacent);

    constexpr double kAbsCorrelationUpperBound = 0.03;
    EXPECT_LT(std::abs(correlation), kAbsCorrelationUpperBound);
}
