#define NOMINMAX
#include <Windows.h>

#include "StaticRay.h"


// Tracing System Configuration
using ColorSystem	= RGBSystem;
using EmissiveType	= ColorSystem::EmissiveType;
using MaterialType	= ColorSystem::MaterialType;
using ColorFilm16	= ColorFilm<HitRecord<Fixed16, ColorSystem>>;


// Trace the scene for an intersection.
// Returns true if an intersection was found.
template <typename SceneType, typename StateType>
static bool Trace(const SceneType& Scene, StateType& State) {
	State.Reset();

	apply([&State](auto&... Shape) { (Shape.HitExterior(State), ...); }, Scene);

	return State._HitFunc ? State._HitFunc() : false;
}

// Illuminate the scene with each light source.
// Calls the supplied function for each photon emitted by each light.
template <typename LightsType, typename LambdaType>
static void Illuminate(const LightsType& Lights, const Real Multiplier, LambdaType Func) {
	const auto visitor = [=](const auto& Light) {
		const auto traces = Light.Traces(Multiplier);
		for (uint64 trace = 0; trace < traces; trace++)
			Func(Light);
	};

	apply([=](const auto&... Light) { (visitor(Light), ...); }, Lights);
}

// Render the scene.
// Light sources emit photons which are transported through the 
// scene and captured when they pass through the virtual lens.
void Render(const path& Filename) {
#if !defined(_DEBUG)
	// Snooze a bit to let the system calm down.
	cout << "Wait..." << endl;
	Sleep(3000);
#endif

	// Rendering parameters
#if !defined(_DEBUG)
	constexpr auto Multiplier = 1e5r;		// Photons per pass ~= Light.Intensity * Multiplier
	constexpr auto Passes     = 1000u;		// Total photons ~= Photons per pass * Passes
	constexpr auto Bounces    = 10u;		// Maximum bounces per photon
	constexpr auto Buffer     = 1ull << 16;	// Photons to buffer between writes

	const     auto Threads    = max(thread::hardware_concurrency(), 1u);
#else
	constexpr auto Multiplier = 1r;
	constexpr auto Bounces    = 1u;
	constexpr auto Passes     = 1u;
	constexpr auto Buffer     = 1ull;

	constexpr auto Threads    = 1u;
#endif

	// Camera setup
	constexpr auto LensRadius = 2r;
	constexpr auto CameraPos  = RVector{-2, 4, 2};
	constexpr auto CameraTgt  = RVector{ 2,-4,-2};
	constexpr auto CameraDir  = (CameraTgt - CameraPos).ConstNormalized();

	// Material colors
	using RedMaterial   = MaterialColor<ColorSystem, {0.9, 0.3, 0.3}>;
	using BlueMaterial  = MaterialColor<ColorSystem, {0.3, 0.3, 0.9}>;
	using WhiteMaterial = MaterialColor<ColorSystem, {0.9, 0.9, 0.9}>;

	// Materials
	using RedPaint   = IdealDiffuse<RedMaterial  >;
	using BluePaint  = IdealDiffuse<BlueMaterial >;
	using WhitePaint = IdealDiffuse<WhiteMaterial>;
	using Mirror     = IdealMirror;

	// Light colors
	using WhiteLight = EmissiveColor<ColorSystem, {1.0, 1.0, 1.0}>;
	using GreenLight = EmissiveColor<ColorSystem, {0.0, 1.0, 0.0}>;

	// Scene setup
	constexpr tuple scene {
		Plane<{ 0, 0,-6}, { 0, 0, 1}, WhitePaint>{},	// Floor
		Plane<{ 0, 0, 6}, { 0, 0,-1}, WhitePaint>{},	// Ceiling
		Plane<{ 0,-6, 0}, { 0, 1, 0}, WhitePaint>{},	// North wall
		Plane<{ 0, 6, 0}, { 0,-1, 0}, WhitePaint>{},	// South wall
		Plane<{-6, 0, 0}, { 1, 0, 0}, RedPaint  >{},	// West wall
		Plane<{ 6, 0, 0}, {-1, 0, 0}, BluePaint >{},	// East wall
		
		Sphere<{-4,-4, 1}, 2r, BluePaint>{},
		Sphere<{ 4,-4, 1}, 2r, RedPaint >{},
		Sphere<{ 0, 0,-3}, 3r, Mirror   >{},
	
		Lens< CameraPos, CameraDir, {0, 0, 1}, LensRadius, 0.8r>{},	// Camera
	};

	// Light sources
	constexpr tuple lights {
#if !defined(_DEBUG)
		OmniSphere<{0, 0, 5}, 1r, 1r,	WhiteLight>{},
		PointLight<{0, 5,-5}, 1r,		GreenLight>{},
#else
		PointBeam<{-1.2,5.5,0.8}, {0,-1,0}, 1r, WhiteLight>{},
#endif
	};

	// Current pass number, synchronized.
	atomic_uint32_t pass = 0;

	// Create the output file.
	DataStream data;
	const auto filename = path("out/") / Filename;
	if (data.New(filename))
		return;

	// Prepare tracer states for each thread.
	using StateType = TraceState<EmissiveType, ColorFilm16>;
	vector<StateType> states(Threads);
	for (auto& state : states) {
		// Seed each thread's RNG with a unique sequence.
		static Random seed;
		seed.LongJump();

		// Initialize the tracing state.
		state = { {&data, Buffer}, seed };
		state.Film.Config = { LensRadius };
	}

	// Write the film configuration.
	if (states[0].Film.WriteConfig())
		return;
	
	// Take the current time.
	const auto start = Mark();

	// Launch worker threads.
	vector<thread> workers;
	for (unsigned worker = 0; worker < Threads; worker++)
		workers.push_back(thread([&](const unsigned worker) {
			// Alias this thread's tracing state.
			auto& state = states[worker];

			// Run this worker until all passes have been completed.
			for (; pass.fetch_add(1u) < Passes;)
				// Illuminate the scene...
				Illuminate(lights, Multiplier,
					[=, &scene, &state](const auto& Light) {
						// Start tracing by emitting a photon.
						Light.Emit(state);

						// Trace and bounce the photon until...
						// it bounces too many times, or
						// no intersections were found, or
						// the trace electively terminates.
						for (Integer bounce = 0; 
							bounce < Bounces && Trace(scene, state); 
							state._Hits++, bounce++);
					});
		}, worker));

	// Wait for all workers to complete.
	for (auto& worker : workers)
		if (worker.joinable())
			worker.join();

	// Measure the time elapsed.
	const auto elapsed = Elapsed(start);

	// Flush remaining output buffers and collect final stats.
	uint64 hits = 0, exposures = 0;
	for (auto& state : states) {
		state.Film.Flush();
		hits += state._Hits;
		exposures += state.Film._Exposures;
	}

	// Close the output file.
	data.Close();

	// Report statistics.
	cout << fixed << setprecision(2);
	cout << exposures << " exposures in " << elapsed << " seconds." << endl;
	cout << hits / 1e6 << "M scene traces @ " << hits / elapsed / 1e6 << "M traces/sec." << endl;
}

// Develop the image.
// Captured photons are loaded and projected through a the
// virtual lens to form a sequence of image files.
void Develop(const path& Filename) {
	// Camera configuration
	constexpr auto Zoom		= 1r;
	constexpr auto FocalLen	= 1r;
	constexpr auto FLimit	= 0.8r;
	
	constexpr auto Width	= 256u;
	constexpr auto Height	= 256u;

	constexpr auto Frames	= 256u;

#if !defined(_DEBUG)
	const     auto Threads = max(thread::hardware_concurrency(), 1u);
#else
	constexpr auto Threads = 1u;
#endif

	// Scan the input file to estimate exposure.
	const auto filename = path("out/") / Filename;
	Real exposure;
	{
		// Open the file in read-only mode.
		DataStream data;
		if (data.Open(filename, true))
			return;

		ColorFilm16 film{&data, 1ULL << 20};

		// Count the stored photons.
		uint64 photons = 0;
		film.ReadHits([&](auto& hits) { photons += hits.size(); });

		// Compute the exposure normalization factor.
		exposure = 2r / (Real(photons) / (Width * Height));
	}

	// Current frame number, synchronized.
	atomic_uint32_t frameIdx = 0;

	// Launch worker threads.
	vector<thread>  workers;
	for (unsigned t = 0; t < Threads; t++)
		workers.push_back(thread([&]([[maybe_unused]] const unsigned id) {
			// Open the file in read-only mode.
			DataStream data;
			if (data.Open(filename, true))
				return;

			ColorFilm16 film{&data, 1ULL << 20};

			// This worker will process a single frame by itself.
			for (unsigned frame; (frame = frameIdx.fetch_add(1u)) < Frames;) {
				// Rewind the data stream and [re]initialize the film.
				if (data.Rewind() || film.ReadConfig())
					continue;

				// Output Image
				RImage image({Width, Height});
				const auto half		= RVector{Width, Height} / 2r;	// Image center.

				// Per-Frame / Animated Parameters
				const auto focalDist = 2r + frame / 32r;

				// Virtual Lens Configuration
				const auto lensRad	= film.Config.LensRadius;
				const auto radSq	= lensRad * lensRad;
				const auto fLimit	= RVector{1, FLimit}.Normalized().y;

				// Scale the virtual image to fit the real image.
				const auto hScale	= half * lensRad * FocalLen * Zoom * csqrt(2r) / -2r;

				// Load all photons from the file.
				film.ReadHits([&](auto& hits) {
					// Process each captured photon.
					for (const auto& hit : hits) {
						// Decode the photon's hit position.
						// Relative to the virtual lens position.
						RVector recPos{hit.Pos.u, hit.Pos.v};
						recPos *= film.Config.LensRadius;

						// Decode the photon's direction.
						// Relative to the virtual lens direction.
						RVector recDir{hit.Dir.u, hit.Dir.v};
						recDir.z = sqrt(1r - recDir.x*recDir.x - recDir.y*recDir.y);

						// Compute deflection at this location on the virtual lens.
						const auto lensDef = RVector{recPos.x, recPos.y, FocalLen}.Normalized();

						// Eliminate photons masked by the aperture.
						if (recDir.Dot(lensDef) < fLimit)
							continue;

						// Add the virtual lens surface normal to the ray direction.
						recDir.z = 1r - recDir.z;

						// Compute the projected ray's new direction.
						const RVector projDir = (recDir - lensDef).Normalized();

						// Compute the distance to the image plane.
						const auto imgDist  = 1r / (1r / FocalLen - 1r / focalDist);

						// Compute where the projected ray intersects the image plane.
						const auto imgPos = recPos + projDir * imgDist / -projDir.z;

						// Normalize and center the image.
						const auto pixel = imgPos * hScale + half;

						// Perform lower boundary checks.
						if (pixel.x < 0r || pixel.y < 0r ||
							isnan(pixel.x) || isinf(pixel.x) ||
							isnan(pixel.y) || isinf(pixel.y))
							continue;

						// Convert to pixel coordinates.
						// Perform upper boundary checks.
						const Coord coord{pixel.x, pixel.y};
						if (coord.x >= image.Dimensions.x ||
							coord.y >= image.Dimensions.y)
							continue;

						// Decode the photon color.
						const auto color = RGBSystem::Load(hit.Clr);
						
						// Accumulate color on this pixel.
						image(coord) += color;
					}
				});

				// Normalize intensity.
				image.ForEach([exposure, &image](const Coord& Pixel) {
					image(Pixel) *= exposure;
				});

				// Report the frame number being written.
				cout << format("{} ", frame);

				// Write the image to disk.
				string filename = format("out/out{:04d}.tga", frame);
				image.Write(filename);
			}
		}, t));

	// Wait for all workers to complete.
	for (auto& worker : workers)
		if (worker.joinable())
			worker.join();
}

// Program entry point
int main() {
	Render("out.dat");
	Develop("out.dat");
}