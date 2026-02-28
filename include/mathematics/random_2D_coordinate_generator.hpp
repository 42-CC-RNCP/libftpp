#pragma once

// Generates pseudo-random numbers based on 2D coordinates.
// What we expect is that
//  1. the output is uniformly distributed across the output space, and
//  2. the output should meet the avalanche effect, i.e. a small change in the
//      input (e.g. changing one bit) should cause a large change in the output
//      (e.g. changing many bits).
//      SAC: a single bit change in the input should cause about 50% of the
//      output bits to change.
class Random2DCoordinateGenerator
{
public:
    long long operator()(const long long& x, const long long& y) const
    {
        // step1: combine x, y, and seed into a single state.
        // each dimension is multiplied by a distinct constant to prevent
        // symmetry collisions (e.g. hash(a,b) == hash(b,a)) and to ensure
        // each axis steps uniformly across the 64-bit integer space.
        long long hash = x;
        hash ^= y * weyl_sequence_;
        hash ^= seed_ * odd_const_;

        // step2: remove input structure via alternating diffusion and confusion
        //
        // each round consists of two operations:
        //   - XOR-shift (diffusion): propagates high-bit entropy downward,
        //       so low bits become dependent on all bits.
        //   - odd multiplication (confusion): a 1-bit difference propagates
        //       upward via carry, spreading ~32 bits of change per round.
        //
        // without this step, nearby coordinates such as (0,0), (1,0), (2,0)
        // would still produce similar outputs because the input difference
        // has not yet avalanched through all 64 bits.
        hash = (hash ^ (hash >> 30)) * odd_const_;

        // second round: further diffusion and confusion to reach full SAC.
        // the XOR-shift propagates remaining high-bit structure downward;
        // the multiplication then re-spreads all bits via carry.
        hash = (hash ^ (hash >> 27)) * weyl_sequence_;

        // final diffusion pass to correct any residual high-bit bias
        hash ^= hash >> 31;
        return hash;
    }
    long long seed() const { return seed_; }

private:
    // Weyl sequence constant can make each step more uniformly distributed
    // the odd constant can help to further mix the bits.
    const long long weyl_sequence_ = 0x9e3779b97f4a7c15ULL;
    const long long odd_const_ = 0xbf58476d1ce4e5b9ULL;
    long long seed_ = 0;
};

/*

Example with 8bits number generator:

X = 0000 0011
Y = 0000 0010

Target:
1. mix X, Y and seed together to get a state that has no structure of the input,
e.g. 1010 1101
2. And the result we want is hash(X, Y) should have 4 bits changed, e.g. 1010
1101, which has 4 bits different from the input.

so base on the solution above, with XOR-shift and const multiplication:

Step1:
(x=3, y=2):
  hash = 0000 0011
  hash ^= 0000 0010 * 1001 1110 = 0011 1100
  result = 0011 1111
(x=2, y=3):
    hash = 0000 0010
    hash ^= 0000 0011 * 1001 1110 = 0101 1010
    result = 0101 1000

(x=3, y=2) → hash = x ^ (y * C)
           = 0000 0011 ^ (2 * C)

(x=2, y=3) → hash = x ^ (y * C)
           = 0000 0010 ^ (3 * C)


the result is different, without collision,
because each dimension has different step size (C and 2C) across the 64-bit
space, so they won't align to cause collisions.

Then we do XOR-shift and multiplication to further mix the bits, to achieve the
avalanche effect.

Let's use (x=192, y=0, seed=0) as an example:
X = 1100 0000 = 192
Y = 0000 0000 = 0
seed = 0

hash = 1100 0000   (= x)
hash ^= 0000 0000 * 1001 1110 = 0000 0000   (y=0, so no change)
result = 1100 0000

Step2.1: XOR-shift >> 2
hash = 1100 0000
hash >> 2 = 0011 0000
XOR       = 1111 0000 (the high bits are propagated down to low bits)

With XOR-shift, the high bits are propagated down to low bits, but the low bits
are not yet fully mixed, so we do multiplication to further mix the bits via
carry, which can cause about 4 bits change for a 1-bit input change.

ODD_CONST(8bit) = 1011 0101, whcih 1-bit is on (0, 2, 4, 5, 7)

1111 0000 << 0 = 1111 0000
1111 0000 << 2 = 1100 0000
1111 0000 << 4 = 0000 0000
1111 0000 << 5 = 0000 0000
1111 0000 << 7 = 0000 0000

=> sum = hash = 1011 0000

Then we do another round of XOR-shift and multiplication to further mix the
bits, to achieve full SAC.

Step 2.2: XOR-shift >> 3
hash     = 1011 0000
hash >> 3= 0001 0110
XOR      = 1010 0110 <- the remaining high-bit structure is propagated down to
low bits

WEYL_CONST(8bit) = 1001 1110, which has 1-bit on (1, 2, 3, 4, 7)
1010 0110 << 1 = 0100 1100
1010 0110 << 2 = 1001 1000
1010 0110 << 3 = 0010 0000
1010 0110 << 4 = 0100 0000
1010 0110 << 7 = 0000 0000

=> sum = hash = 0100 0100

Finally, we do one more XOR-shift to correct any residual high-bit bias, to get
the final result.

Step 2.3: XOR-shift >> 4
hash     = 0100 0100
hash >> 4= 0000 0100
XOR      = 0100 0000

input:  1100 0000, (x=192, y=0, seed=0)
output: 0100 0000

8bits is not enough to show the full effect, but with 64bits, we can achieve
about 32 bits change for a 1-bit input change, which meets the SAC requirement.

*/
