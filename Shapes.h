#pragma once


// Basic Shapes ===========================================


template <RVector Position, Real Radius, typename Material>
struct Sphere {
	static constexpr RVector _InvRad = 1r / Radius;
	static constexpr Real	 _RadSq  = Radius * Radius;

	// Detect an intersection with the exterior of the sphere.
	template <typename StateType>
	void HitExterior(StateType& State) const {
		const auto dlt = Position - State.Position;
		const auto adj = dlt.Dot(State.Direction);
		if (adj < Epsilon)
			return;

		const auto oppSq = dlt.LengthSq() - adj * adj;
		if (oppSq >= _RadSq)
			return;

		const auto dist = adj - sqrt(_RadSq - oppSq);
		if (dist >= State._HitDist)
			return;

		State.Hit(dist, [=, &State]() -> bool {
			State.Position += State.Direction * State._HitDist;
			return Material::Interface(State, *this);
		});
	}

	// Compute the surface normal at the hit position.
	template <typename StateType>
	inline void HitNormal(StateType& State) const {
		State._HitNorm = (State.Position - Position) * _InvRad;
	}
};

template <RVector Position, RVector Normal, typename Material>
struct Plane {
	// Detect an intersection with the exterior of the plane.
	template <typename StateType>
	void HitExterior(StateType& State) const {
		auto dist = Normal.Dot(State.Direction);
		if (dist > -Epsilon)
			return;

		dist = Normal.Dot(Position - State.Position) / dist;
		if (dist >= State._HitDist || dist < Epsilon)
			return;

		State.Hit(dist, [=, &State]() -> bool {
			State.Position += State.Direction * State._HitDist;
			return Material::Interface(State, *this);
		});
	}

	// Return the surface normal.
	template <typename StateType>
	inline void HitNormal(StateType& State) const {
		State._HitNorm = Normal;
	}
};