#pragma once


// Basic Types ============================================


using int8    = signed char;
using int16   = signed short;
using int32   = signed int;
using int64   = signed long long;

using uint8   = unsigned char;
using uint16  = unsigned short;
using uint32  = unsigned int;
using uint64  = unsigned long long;

using float32 = float;
using float64 = double;
using float80 = long double;

using Integer = int;
using Real    = float;


// "Real" Notation (r) ====================================
// Both integer and floats are cast as to Real.
// Examples: 1r, 0x1p23r, 1.23r, 1.23e45r


#pragma warning(suppress : 4455)
static constexpr Real operator""r(long double Value) noexcept {
	return Real(Value);
}

#pragma warning(suppress : 4455)
static constexpr Real operator""r(unsigned long long int Value) noexcept {
	return Real(int64(Value));
}


// Constants ==============================================


constexpr Real Epsilon  = 0x1p-22r;
constexpr Real Infinity = INFINITY;