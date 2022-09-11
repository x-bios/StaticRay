#pragma once


// Imaging Film ===========================================


// 16bit Fixed-Width Float
// Evenly distributed across a range of -1 to +1.
// Values out of bounds are clamped.
struct Fixed16 {
	int16	Value;

	Fixed16() = default;

	// Convert from floating-point to int16.
	template <typename Type>
	requires is_floating_point_v<Type>
	inline Fixed16(const Type Value) :
		Value(int16(clamp(int32(Value * 32768r), int32(-32768), int32(32767)))) {}

	// Convert from int16 to Real.
	inline operator Real() const {
		return Real(Value) / 32768r;
	}
};

// Photon Hit Record (Compact Storage Format)
template <typename CoordType, typename ColorSystem>
struct HitRecord {
	using ColorType = ColorSystem::EmissiveType;

	struct { CoordType u, v; } Pos;	// Hit Position
	struct { CoordType u, v; } Dir;	// Ray Direction
	ColorSystem::StorageType   Clr;	// Photon Color

	HitRecord() = default;
	
	template <typename UVType>
	HitRecord(const UVType UPos, const UVType VPos, const UVType UDir, const UVType VDir, const ColorType& Color) :
		Pos{CoordType(UPos), CoordType(VPos)}, Dir{CoordType(UDir), CoordType(VDir)}, Clr(ColorSystem::Store(Color)) {}
};

enum BlockTags {
	TAG_Config	= 1,	// Camera Configuration
	TAG_Hits	= 2,	// Photon Hit Records
};

// Simple Digital Film
template <typename HitType>
struct ColorFilm : vector<HitType> {
	// Virtual Camera Configuration
	struct ConfigHeader : BlockHeader {
		float32	LensRadius;

		ConfigHeader(const float32 LensRadius = 0r) :
			BlockHeader(TAG_Config, sizeof ConfigHeader),
			LensRadius(LensRadius) {}

		inline bool Validate() const {
			return BlockHeader::Validate(TAG_Config, sizeof ConfigHeader);
		}
	} Config;

	// Photon Hit Record Storage
	struct FilmHeader : BlockHeader {
		uint32	Count;

		FilmHeader(const uint32 Count = 0r) :
			BlockHeader(TAG_Hits, sizeof FilmHeader + sizeof HitType * Count),
			Count(Count) {}

		inline bool Validate() const {
			return BlockHeader::Validate(TAG_Hits,
				sizeof FilmHeader + sizeof HitType * Count);
		}
	};

	DataStream* Stream = nullptr;	// Pointer to the data streamer.
	uint64		_Exposures = 0;		// Statistics: Exposures recorded.

	ColorFilm() = default;

	ColorFilm(DataStream* Stream, const size_t BufferLimit) : Stream(Stream) {
		assert(Stream && BufferLimit && BufferLimit < (1ULL << 32));
		this->reserve(BufferLimit);
	}

	// Expose the digital film to the photon.
	// Returns true on error.
	bool Expose(HitType&& Hit) {
		// Encode and buffer the captured photon.
		this->push_back(forward<HitType>(Hit));

		// Flush the buffer when full.
		assert(this->size() < (1ULL << 32));
		return this->size() != this->capacity() || Flush();
	}

	// Write all buffered photons to the data stream.
	// Returns true on error.
	bool Flush() {
		assert(Stream && this->size() < (1ULL << 32));
		const auto hits = uint32(this->size());
		_Exposures += hits;

		// Prepare the block header.
		const FilmHeader hdr(hits);
		
		// Obtain ownership of the data stream.
		auto sync = Stream->Sync();

		// Write the hit record block.
		if (Stream->WriteHeader(hdr) ||
			Stream->Write(this->data(), hits))
			return true;

		// Empty the buffer.
		this->resize(0);

		return false;
	}

	// Read a block of hit records.
	// Returns true on error.
	bool Read() {
		assert(Stream);

		// Obtain ownership of the data stream.
		auto sync = Stream->Sync();

		// Seek to the next block of hit records.
		FilmHeader hdr;
		if (Stream->Seek(TAG_Hits) ||
			Stream->ReadHeader(hdr))
			return true;

		// Prepare the hit record buffer.
		this->resize(hdr.Count);

		// Read the hit records.
		return Stream->Read(this->data(), hdr.Count);
	}

	// Write the virtual camera configuration.
	// Returns true on error.
	inline bool WriteConfig() const {
		assert(Stream);
		auto sync = Stream->Sync();
		return Stream->WriteHeader(Config);
	}

	// Read the virtual camera configuration.
	// Returns true on error.
	inline bool ReadConfig() {
		assert(Stream);
		auto sync = Stream->Sync();
		return Stream->Seek(TAG_Config) || Stream->ReadHeader(Config);
	}

	// Call the supplied function on each block of hit records.
	template <typename LambdaFunc>
	inline void ReadHits(LambdaFunc Func) {
		for (; !Read(); Func(*this));
	}
};