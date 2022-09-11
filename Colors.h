#pragma once


// Color Systems ==========================================


// Basic RGB color system
struct RGBSystem {
	using EmitterType  = RColor;
	using EmissiveType = RColor;
	using MaterialType = RColor;
	using StorageType  = BColor;

	// Stop tracing if the photon dims substantially.
	static constexpr auto LumaCutoff = 0.001r;

	// Use the emitter to select an emissive color to emit.
	inline static void Emit(EmissiveType& Color, const EmitterType& Emitter) {
		Color = Emitter;
	}

	// Diminish the emissive color on material interactions.
	inline static bool Absorb(EmissiveType& Color, const MaterialType& Material) {
		return (Color *= Material).Sum() < LumaCutoff;
	}

	// Convert the emissive color to its storage format.
	inline static StorageType Store(const EmissiveType& Color) {
		return {(Color * 255r).Clamp4(0r, 255r), 0};
	}

	// Restore a stored emissive color.
	inline static EmissiveType Load(const StorageType& Color) {
		return EmissiveType(Color) / 255r;
	}
};


// Color / System Associator ==============================


template <typename ColorSystem, ColorSystem::EmissiveType ColorValue>
struct EmissiveColor {
	static constexpr auto Color = ColorValue;
	using System = ColorSystem;
};

template <typename ColorSystem, ColorSystem::MaterialType ColorValue>
struct MaterialColor {
	static constexpr auto Color = ColorValue;
	using System = ColorSystem;
};