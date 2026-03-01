#pragma once
#include "ivector2.hpp"
#include "random_2D_coordinate_generator.hpp"
#include <cmath>

class PerlinNoise2D
{
public:
    PerlinNoise2D() = default;
    PerlinNoise2D(const long long seed) : rng_() { rng_.set_seed(seed); }
    ~PerlinNoise2D() = default;

    float sample(float x, float y) const
    {
        IVector2<float> p(x, y);

        // Determine grid cell coordinates
        IVector2<float> c00(std::floor(p.x), std::floor(p.y));
        IVector2<float> c10 = c00 + IVector2<float>(1, 0);
        IVector2<float> c01 = c00 + IVector2<float>(0, 1);
        IVector2<float> c11 = c00 + IVector2<float>(1, 1);

        // Generate random gradients at the corners of the cell
        auto grad00 = _grad(c00.x, c00.y);
        auto grad10 = _grad(c10.x, c10.y);
        auto grad01 = _grad(c01.x, c01.y);
        auto grad11 = _grad(c11.x, c11.y);

        // Compute distance vectors from the corners to the point(x, y)
        IVector2<float> s00 = p - c00;
        IVector2<float> s10 = p - c10;
        IVector2<float> s01 = p - c01;
        IVector2<float> s11 = p - c11;

        // Compute dot products between gradients and distance vectors
        float dot00 = s00.dot(grad00);
        float dot10 = s10.dot(grad10);
        float dot01 = s01.dot(grad01);
        float dot11 = s11.dot(grad11);

        // Compute fade curves for x and y
        float sx = _fade(p.x - static_cast<float>(c00.x));
        float sy = _fade(p.y - static_cast<float>(c00.y));

        // Interpolate the results
        float n0 = dot00 + (dot10 - dot00) * sx;
        float n1 = dot01 + (dot11 - dot01) * sx;
        return n0 + (n1 - n0) * sy;
    }

private:
    // fade function as defined by Ken Perlin. This eases the coordinate values
    // formula: 6t^5 - 15t^4 + 10t^3
    float _fade(float t) const { return t * t * t * (t * (t * 6 - 15) + 10); }

    // pseudo-random gradient value in [-1, +1]
    IVector2<float> _grad(float x, float y) const
    {
        long long hash =
            rng_(static_cast<long long>(x), static_cast<long long>(y));

        // Map the hash to an angle in [0, 2π)
        float angle = hash * 2.0f * static_cast<float>(M_PI);

        // Convert the angle to a unit vector (cos(angle), sin(angle))
        return IVector2<float>(std::cos(angle), std::sin(angle));
    }

private:
    Random2DCoordinateGenerator rng_;
};
