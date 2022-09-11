#pragma once


// Light Sources ==========================================


template <Real Intensity, typename Color>
struct LightBase {
	// Compute the number of photons to emit.
	constexpr uint64 Traces(const Real Multiplier) const {
		return uint64(Intensity * Multiplier);
	}

	// Emit a colored photon.
	template <typename StateType>
	inline void EmitColor(StateType& State) const {
		Color::System::Emit(State.Color, Color::Color);
	}
};

// Debug Beam Emitter
template <RVector Position, RVector Direction, Real Intensity, typename Color>
struct PointBeam : LightBase<Intensity, Color> {
	// Emit a photon.
	template <typename StateType>
	void Emit(StateType& State) const {
		State.Position	= Position;
		State.Direction	= Direction;
		
		this->EmitColor(State);
	}
};

// Omni-Directional Point Light
template <RVector Position, Real Intensity, typename Color>
struct PointLight : LightBase<Intensity, Color> {
	// Emit a photon.
	template <typename StateType>
	void Emit(StateType& State) const {
		State.Position	= Position;
		State.Direction	= RandomNormal(State.RNG);

		this->EmitColor(State);
	}
};

// Omni-Directional Spherical Light
template <RVector Position, Real Radius, Real Intensity, typename Color>
struct OmniSphere : LightBase<Intensity, Color> {
	// Emit a photon.
	template <typename StateType>
	void Emit(StateType& State) const {
		const auto dir	= RandomNormal(State.RNG);
		State.Position	= Position + dir * Radius;
		State.Direction	= (dir + RandomNormal(State.RNG)).Normalized();
		
		this->EmitColor(State);
	}
};