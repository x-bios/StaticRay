#pragma once


// Virtual Lens ===========================================


template <
	RVector		Position,		// Worldspace position.
	RVector		Direction,		// Forward direction.
	RVector		Up,				// Up direction.
	Real		Aperture,		// Diameter of the aperture.
	Real		FLimit>			// Maximum F-number to capture.
struct Lens {
	static constexpr Real		_FLim = RVector(1, -FLimit).ConstNormalized().y;// Cosine of the F-limit.
	static constexpr Real		_RadSq = (Aperture * Aperture) / 4r;			// Square of the aperture's radius.
	static constexpr RVector	_U = Direction.Cross(Up).ConstNormalized();		// +U axis, normalized.
	static constexpr RVector	_V = Direction.Cross(_U);						// +V axis, normalized.
	static constexpr RVector	_Ua = _U / Aperture / 2r;						// U axis scaled to aperture.
	static constexpr RVector	_Va = _V / Aperture / 2r;						// V axis scaled to aperture.

	template <typename StateType>
	void HitExterior(StateType& State) const {
		// Project the lens direction on the ray direction.
		const auto proj = Direction.Dot(State.Direction);
		
		// Ignore photons beyond the F-limit.
		if (proj > _FLim)
			return;

		// Distance to the intersection on the lens plane.
		const auto dist = Direction.Dot(Position - State.Position) / proj;

		// Ignore photons that:
		// - have hit something nearer than the lens,
		// - or are nearly coplanar with the lens.
		if (dist >= State._HitDist || dist < Epsilon)
			return;

		// Compute the final intersection position.
		const auto pos = State.Position + State.Direction * dist;

		// Ignore intersections outside the lens's radius.
		if ((pos - Position).LengthSq() >= _RadSq)
			return;

		// Capture the photon.
		State.Hit(dist, [=, &State]() -> bool {
			State.Position = pos;

			// Transform the photon to filmspace and capture it.
			State.Film.Expose({_Ua.Dot(pos), _Va.Dot(pos),
				_U.Dot(State.Direction), _V.Dot(State.Direction),
				State.Color});

			// Tracing continues.
			return false;
		});
	}
};