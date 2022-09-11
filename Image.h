#pragma once


// Image Template Class ===================================


// Pixel Coordinates
struct Coord {
	Integer	x, y;

	Coord() = default;

	template <typename XType, typename YType>
	Coord(const XType& x, const YType& y) :
		x(Integer(x)), y(Integer(y)) {}

	[[nodiscard]] Coord operator+ (const Coord& Other) const {
		return { x + Other.x, y + Other.y };
	}

	[[nodiscard]] Coord operator- (const Coord& Other) const {
		return { x - Other.x, y - Other.y };
	}

	[[nodiscard]] Coord operator* (const Coord& Other) const {
		return { x * Other.x, y * Other.y };
	}

	[[nodiscard]] Coord operator/ (const Coord& Other) const {
		return { x / Other.x, y / Other.y };
	}
};


// Basic Image Template
template <typename Type>
struct ImageType : vector<Type> {
	Coord	Dimensions;

	ImageType(Coord&& Dimensions) {
		Resize(forward<Coord>(Dimensions));
	}

	void resize(const size_t) = delete;

	void Resize(Coord&& Dimensions) {
		this->Dimensions = move(Dimensions);
		vector<Type>::resize(Dimensions.x * Dimensions.y);
	}

	[[nodiscard]] inline Type& operator() (const Coord& Coord) {
		return (*this)[Coord.y * Dimensions.x + Coord.x];
	}

	[[nodiscard]] inline const Type& operator() (const Coord& Coord) const {
		return (*this)[Coord.y * Dimensions.x + Coord.x];
	}

	// Iterate over every pixel in the image.
	// This proceeds from bottom to top, right to left.
	template <typename LambdaFunc>
	void ForEach(const LambdaFunc Func) {
		for (Coord coord{0, Dimensions.y}; --coord.y >= 0;)
			for (coord.x = Dimensions.x; --coord.x >= 0;)
				Func(coord);
	}
};


// Image Template with Targa Output
template <typename PixelType>
class TargaType : public ImageType<PixelType> {
protected:
#pragma pack(push, 1)
	// Targa File Header
	struct TGAHeader {
		uint16	IDLen_MapType;
		uint8	TypeCode = 2;
		uint32	ClrMapOrg_Len;
		uint8	MapEntrySize;
		uint32	XOrg_YOrg;
		uint16	Width, Height;
		uint8	BPP, ImgDesc;
	};
#pragma pack(pop)

public:
	using ImageType<PixelType>::ImageType;

	// Write the image as a 24 or 32bit Targa file.
	// Returns true on error.
	template <int Channels = 3>
	bool Write(const string& Filename) {
		// Open the output file for writing.
		ofstream file(Filename, ios::binary | ios::trunc);
		if (!file.is_open())
			return true;

		// Write the Targa header.
		TGAHeader header{};
		header.Width  = this->Dimensions.x;
		header.Height = this->Dimensions.y;
		header.BPP    = Channels * 8;
		file.write((const char*)&header, sizeof TGAHeader);
		if (file.bad())
			return true;

		// Write pixels.
		this->ForEach(
			[&](const Coord& Coord) {
				const auto color = (*this)(Coord).Color();
				file.write((const char*)&color, Channels);
			});

		// Close the file. Done!
		file.close();
		return false;
	}
};


// 4xReal Floating-Point Image
// Saves to a 24/32bit Targa file.
using RImage  = TargaType<RColor>;

// 4x32bit Integer Image
// Saves to a 24/32bit Targa file.
using IImage  = TargaType<IColor>;

// 4x8bit Integer Image
// Saves to a 24/32bit Targa file.
using BImage  = TargaType<BColor>;