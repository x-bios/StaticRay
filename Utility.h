#pragma once


// Benchmarking Utilities =================================


// High-Precision Timestamp
using timestamp = chrono::high_resolution_clock::time_point;

// Return the current time at high precision.
inline timestamp Now() {
	return chrono::high_resolution_clock::now();
}

// Return the current time after the next tick.
inline timestamp Mark() {
	timestamp start = Now(), now;
	for (; (now = Now()) == start;);
	return now;
}

// Return the time elapsed since the specified time point.
inline double Elapsed(const timestamp& Since) {
	return chrono::duration_cast<chrono::duration<double>>(Now() - Since).count();
}


// Math Utilities =========================================


// Make a random 3D vector in the range [0..1) in ~4.21ns (~1.4ns/ea).
// Provides only 21 of 23 possible bits of randomness each.
inline FVector RandomXYZUnsigned(const uint64 Bits) {
	auto v = IVector{Bits << 1, Bits >> 20, Bits >> 41} & 0x007FFFFCUL | 0x3F800000UL;
	return *(FVector*)&v - 1.f;
}

// Make a random 4D vector in the range [0..1) in ~5.31ns (~1.33ns/ea).
// Provides only 16 of 23 possible bits of randomness each.
inline FVector RandomXYZWUnsigned(const uint64 Bits) {
	auto v = IVector{Bits << 7, Bits >> 9, Bits >> 25, Bits >> 41} & 0x007FFF80UL | 0x3F800000UL;
	return *(FVector*)&v - 1.f;
}

// Make a random 3D vector in the range (-1..+1) in ~5.36ns (~1.77ns/ea).
// Provides only 20 of 23 possible bits of randomness each.
inline FVector RandomXYZSigned(const uint64 Bits) {
	auto v = IVector{Bits << 2, Bits >> 18, Bits >> 38} & 0x007FFFF8UL | 0x3F800000UL;
	*(FVector*)&v -= 1; v |= IVector{Bits >> 32, Bits >> 31, Bits >> 30} & 0x80000000UL;
	return *(FVector*)&v;
}

// Make a random 3D vector evenly distributed within a unit sphere.
inline RVector RandomInSphere(Random& RNG) {
	for (;;) {
		const auto point = RandomXYZSigned(RNG());
		if (point.LengthSq() < 1r)
			return point;
	}
}

// Make a random 3D unit vector.
inline RVector RandomNormal(Random& RNG) {
	return RandomInSphere(RNG).Normalized();
}