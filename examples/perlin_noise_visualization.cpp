#include "mathematics/perlin_noise_2D.hpp"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <string>

namespace
{

int to_byte(float value)
{
    value = std::clamp(value, 0.0f, 1.0f);
    return static_cast<int>(std::lround(value * 255.0f));
}

float fractal_noise(const PerlinNoise2D& noise,
                    float x,
                    float y,
                    int octaves,
                    float persistence,
                    float lacunarity)
{
    float amplitude = 1.0f;
    float frequency = 1.0f;
    float sum = 0.0f;

    for (int octave = 0; octave < octaves; ++octave) {
        sum += noise.sample(x * frequency, y * frequency) * amplitude;
        amplitude *= persistence;
        frequency *= lacunarity;
    }

    return sum;
}

float max_amplitude_sum(int octaves, float persistence)
{
    float amplitude = 1.0f;
    float amplitude_sum = 0.0f;

    for (int octave = 0; octave < octaves; ++octave) {
        amplitude_sum += amplitude;
        amplitude *= persistence;
    }

    return amplitude_sum;
}

} // namespace

int main(int argc, char** argv)
{
    const long long seed = (argc >= 2) ? std::stoll(argv[1]) : 20260301LL;
    const std::string output_path = (argc >= 3) ? argv[2] : "perlin_noise.ppm";

    constexpr int width = 512;
    constexpr int height = 512;
    constexpr float sample_scale = 0.02f;
    constexpr int octaves = 5;
    constexpr float persistence = 0.5f;
    constexpr float lacunarity = 2.0f;

    const PerlinNoise2D noise(seed);
    const float amplitude_sum = max_amplitude_sum(octaves, persistence);

    std::ofstream out(output_path, std::ios::binary);
    if (!out.is_open()) {
        std::cerr << "Failed to open output file: " << output_path << '\n';
        return 1;
    }

    out << "P3\n" << width << ' ' << height << "\n255\n";

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const float nx = static_cast<float>(x) * sample_scale;
            const float ny = static_cast<float>(y) * sample_scale;

            const float raw =
                fractal_noise(noise, nx, ny, octaves, persistence, lacunarity);
            const float normalized =
                (raw + amplitude_sum) / (2.0f * amplitude_sum);
            const int gray = to_byte(normalized);

            out << gray << ' ' << gray << ' ' << gray << '\n';
        }
    }

    std::cout << "Generated " << output_path << " with seed=" << seed << '\n';
    std::cout << "Preview: open with any image viewer that supports PPM.\n";

    return 0;
}
