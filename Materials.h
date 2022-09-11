#pragma once


// Materials ==============================================


template <typename ColorType>
struct IdealDiffuse {
	template <typename StateType, typename ShapeType>
	static bool Interface(StateType& State, const ShapeType& Shape) {
		// Will this photon be absorbed?
		if (ColorType::System::Absorb(State.Color, ColorType::Color))
			// If so, terminate the trace.
			return false;

		// Compute the surface normal (_HitNorm).
		Shape.HitNormal(State);

		// Compute Lambertian reflection.
		State.Direction = (State._HitNorm + RandomNormal(State.RNG)).Normalized();
		
		// Continue tracing.
		return true;
	}
};

struct IdealMirror {
	template <typename StateType, typename ShapeType>
	static bool Interface(StateType& State, const ShapeType& Shape) {
		// Compute the surface normal (_HitNorm).
		Shape.HitNormal(State);

		// Compute a perfect reflection.
		State.Direction -= State._HitNorm * State.Direction.Dot(State._HitNorm) * 2r;

		// Continue tracing.
		return true;
	}
};

template <typename ColorType, Real Specular>
struct ShinyOpaque {
	template <typename StateType, typename ShapeType>
	static bool Interface(StateType& State, const ShapeType& Shape) {
		// Compute the surface normal (_HitNorm).
		Shape.HitNormal(State);

		if (State.PoolRNG() <= Specular)
			// Specular reflection.
			State.Direction -= State._HitNorm * State.Direction.Dot(State._HitNorm) * 2r;
		else if (!ColorType::System::Absorb(State.Color, ColorType::Color))
			// Diffuse reflection.
			State.Direction = (State._HitNorm + RandomNormal(State.RNG)).Normalized();
		else
			// Photon was absorbed. Terminate the trace.
			return false;

		// Continue tracing.
		return true;
	}
};