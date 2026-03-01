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
    void set_seed(const long long seed) { seed_ = seed; }

private:
    // Weyl sequence constant can make each step more uniformly distributed
    // the odd constant can help to further mix the bits.
    const long long weyl_sequence_ = 0x9e3779b97f4a7c15ULL;
    const long long odd_const_ = 0xbf58476d1ce4e5b9ULL;
    long long seed_ = 0;
};

/*

8-bit illustration (x=3, y=2, seed=0):
  WEYL_CONST(8-bit) = 1001 1110
  ODD_CONST (8-bit) = 1011 0101

Note: this example illustrates the mechanism of diffusion and confusion only.
True SAC requires 64-bit constants (~32 of 64 bits set), which provide enough
carry propagation across all bit positions. With only 8 bits, the effect is
visible but weaker.

--- Step 1: combine x and y into a single state ---

  hash  = 0000 0011             (= x = 3)
  y * WEYL = 2 * 1001 1110
           = 0011 1100          (= 60)
  hash ^= 0011 1100
        = 0011 1111             (= 63)

Symmetry check: hash(3,2) != hash(2,3) because x contributes directly
  while y is scaled by WEYL, giving each axis a different step size.

--- Step 2.1: XOR-shift >> 2 (diffusion: high bits → low bits) ---

  hash        = 0011 1111
  hash >> 2   = 0000 1111
  XOR         = 0011 0000      ← bits 4-5 now carry info from bits 6-7

  * ODD_CONST = 1011 0101, 1-bits at positions: [0, 2, 4, 5, 7]

  0011 0000 << 0 = 0011 0000
  0011 0000 << 2 = 1100 0000
  0011 0000 << 4 = 0000 0000   (overflows 8-bit)
  0011 0000 << 5 = 0000 0000   (overflows 8-bit)
  0011 0000 << 7 = 0000 0000   (overflows 8-bit)

  sum = 0011 0000 + 1100 0000
      = 1111 0000               (= 240, carry from bit 6 flips bit 7)

  hash = 1111 0000

The carry during addition is the key: a difference in low bits propagates
  upward through addition, spreading the change across more bit positions.

--- Step 2.2: XOR-shift >> 3 (diffusion) then multiply (confusion) ---

  hash        = 1111 0000
  hash >> 3   = 0001 1110
  XOR         = 1110 1110      ← high-bit structure now visible in low bits

  * WEYL_CONST = 1001 1110, 1-bits at positions: [1, 2, 3, 4, 7]

  1110 1110 << 1 = 1101 1100   (= 220)
  1110 1110 << 2 = 1011 1000   (= 184)
  1110 1110 << 3 = 0111 0000   (= 112)
  1110 1110 << 4 = 1110 0000   (= 224)
  1110 1110 << 7 = 0000 0000   (overflows 8-bit)

  sum = 220 + 184 + 112 + 224 = 740 mod 256
      = 1110 0100               (= 228)

  hash = 1110 0100

--- Step 2.3: final XOR-shift >> 4 (correct residual high-bit bias) ---

  hash        = 1110 0100
  hash >> 4   = 0000 1110
  XOR         = 1110 1010      (= 234)

--- Result ---

  hash(3, 2) = 1110 1010

--- SAC check: flip bit 0 of x (x=3 → x=2) ---

  hash(3, 2) = 1110 1010
  hash(2, 2) = 0011 1101
  XOR diff   = 1101 0111      ← 6 out of 8 bits changed (75%)

  In 64-bit this stabilises at ~50% (32 of 64 bits), satisfying SAC.
  The 8-bit result is noisier due to limited carry propagation range.

*/
