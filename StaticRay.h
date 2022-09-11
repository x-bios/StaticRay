#pragma once


// Standard Library Includes ==============================


#include <algorithm>
#include <array>
#include <atomic>
#include <cassert>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <format>
#include <fstream>
#include <functional>
#include <iostream>
#include <mutex>
#include <random>
#include <sstream>
#include <thread>
#include <variant>
#include <vector>

using namespace std;
using namespace filesystem;


// Application-Specific Libraries =========================


#include "Types.h"
#include "Vector.h"
#include "Image.h"
#include "Xoroshiro.h"
#include "Utility.h"
#include "Stream.h"
#include "Film.h"
#include "Colors.h"
#include "Materials.h"
#include "Shapes.h"
#include "Lens.h"
#include "Lights.h"


// Trace State ============================================


template <typename ColorType, typename FilmType>
struct TraceState {
	using FuncFunc  = function<bool(void)>;

	FilmType	Film;				// Imaging film (shared across threads).
	Random		RNG;				// Random number generator for this thread.

	RVector		Position;			// Current position of the photon.
	RVector		Direction;			// Current direction of the photon.
	ColorType	Color;				// Current color of the photon.

	RVector		_PoolRand;			// A small pool of random floats.
	Integer		_PoolIndex = 0;		// Current in random pool index.

	Integer		_Hits = 0;			// Statistics: Hit counter.
	Real		_HitDist;			// Distance to the nearest intersection.
	RVector		_HitNorm;			// Surface normal of the intersected shape.
	FuncFunc	_HitFunc;			// Callback for the nearest intersection.

	// Reset the trace for the next bounce.
	inline void Reset() {
		_HitDist = Infinity;
		_HitFunc = nullptr;
	}

	// Update state when a nearer shape is intersected.
	inline void Hit(const Real Distance, FuncFunc Interface) {
		_HitDist = Distance;
		_HitFunc = Interface;
	}

	// Returns a random Real in the range [0..1).
	// Uses a pool and regenerates as needed.
	Real PoolRNG() {
		// Retrieve and update the current pool index.
		const auto idx = _PoolIndex++ & 3;
		
		// When appropriate, regenerate the pool.
		if (!idx)
			_PoolRand = RandomXYZWUnsigned(RNG());

		// Return the random number.
		return _PoolRand[idx];
	}
};