#pragma once


// Xoroshiro Pseudorandom Number Generator ================
// http://xoshiro.di.unimi.it/ or https://prng.di.unimi.it/


// Bitwise circular left shift.
inline uint64 rotl(const uint64 x, const int k) {
	return (x << k) | (x >> (64 - k));
}

// A random number generator with 64bit internal state.
// Based on http://xoroshiro.di.unimi.it/splitmix64.c
struct Random64 {
	uint64	State;

	Random64(const uint64 Seed = 0x1234567890ABCDEFull) : 
		State(Seed) {}

	uint64 operator() () {
		uint64 z = (State    += 0x9E3779B97F4A7C15ull);
		z =   (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
		z =   (z ^ (z >> 27)) * 0x94D049BB133111EBull;
		return z ^ (z >> 31);
	}
};

// A random number generator with 128-bit internal state.
// Based on: http://xoroshiro.di.unimi.it/xoroshiro128plus.c
struct Random128 {
	array<uint64, 2> State;

	Random128(const uint64 Seed = 0x1234567890ABCDEFull) {
		Random64 seeder(Seed);
		State = {seeder(), seeder()};
	}

	uint64 operator() () {
		const uint64 s0 = State[0];
		      uint64 s1 = State[1];
		const uint64 v  = s0 + s1;

		s1 ^= s0;
		State = { rotl(s0, 24) ^ s1 ^ (s1 << 16), rotl(s1, 37) };

		return v;
	}

	// This is equivalent to 2^64 calls to operator(). It can be used to 
	// generate 2^64 non-overlapping subsequences for parallel computations.
	void ShortJump() {
		Jump({ 0xDF900294D8F554A5, 0x170865DF4B3201FC });
	}

	// This is equivalent to 2^96 calls to operator(). It generates 2^32 
	// starting points from which ShortJump() will generate 2^32 non-
	// overlapping subsequences for parallel distributed computations.
	void LongJump() {
		Jump({ 0xD2A98B26625EEE7B, 0xDDDF9B1090AA7AC1 });
	}

protected:
	void Jump(array<uint64, 2>&& Jump) {
		uint64 s0 = 0, s1 = 0;

		for(const uint64 j : Jump)
			for(int b = 0; b < 64; (*this)(), b++)
				if (j & 1ull << b) {
					s0 ^= State[0];
					s1 ^= State[1];
				}

		State = {s0, s1};
	}
};


// Prefer the 128bit RNG.
using Random = Random128;