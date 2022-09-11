#pragma once


// Tagged Data Stream File Wrapper ========================
// Data is stored in a file as a sequence of blocks. Each 
// block contains a BlockHeader and optional user header. 
// The header indicates its size and identifies the block 
// type with a tag. The user must read/write the block in 
// its entirety. The file begins with a fixed header block
// and all other blocks are read/written thereafter. The 
// wrapper facilitates seeking, reading, and writing block 
// headers and user data.


struct DataStream {
	static constexpr uint16	BlockMagic   = 'ST';	// "TS" for Tagged Stream
	static constexpr uint8	FileIdent    = 0;
	static constexpr uint8	VersionMajor = 1;
	static constexpr uint8	VersionMinor = 1;

	// Block Header / User Header Base Class
	struct BlockHeader {
		uint16		Magic;		// Magic header value.
		uint16		Ident;		// Block type identifier.
		uint32		Size;		// Block size in bytes. 4gb-1 limit.

		BlockHeader() = default;
		
		BlockHeader(const uint16 Ident, const size_t Size) :
			Magic(BlockMagic), Ident(Ident), Size(uint32(Size)) {
			assert(Size < (1ULL << 32));
		}

		[[nodiscard]] inline bool Validate(
			const uint16 RequiredIdent, 
			const uint32 RequiredSize) const {
			return Validate() || 
				   Ident != RequiredIdent || 
				   Size  != RequiredSize;
		}

		[[nodiscard]] inline bool Validate() const {
			return Magic != BlockMagic;
		}
	};

	// Fixed File Header
	struct FileHeader : BlockHeader {
		FileHeader() : 
			BlockHeader(FileIdent, sizeof FileHeader) {}

		struct {
			uint8	Major = VersionMajor;
			uint8	Minor = VersionMinor;
		} Version;

		[[nodiscard]] inline bool Validate() const {
			return BlockHeader::Validate(FileIdent, sizeof FileHeader) || 
				Version.Major != VersionMajor || 
				Version.Minor != VersionMinor;
		}
	};

	mutex	_Lock;		// Synchronization mechanism
	fstream	_File{};	// File stream

	~DataStream() {
		// Ensure the file is closed on destruction.
		if (_File.is_open())
			_File.close();
	}

	// Obtain ownership of the stream's mutex.
	// Discard the return value to release ownership.
	[[nodiscard]] inline lock_guard<mutex> Sync() {
		return lock_guard<mutex>(_Lock);
	}

	// Create a new file for writing.
	// Returns true on error.
	bool New(const path& Filename) {
		assert(Filename.has_filename() && !_File.is_open());

		_File.open(Filename, ios::in | ios::out | ios::binary | ios::trunc);
		return !_File.is_open() || Write(FileHeader{});
	}

	// Open an existing file and seek to the end.
	// Returns true on error.
	bool Append(const path& Filename) {
		assert(Filename.has_filename() && !_File.is_open());

		FileHeader hdr;
		_File.open(Filename, ios::in | ios::out | ios::binary);
		return !_File.is_open() || ReadHeader(hdr) || SeekTail();
	}

	// Open an existing file, optionally in read-only mode.
	// Returns true on error.
	bool Open(const path& Filename, const bool ReadOnly = false) {
		assert(Filename.has_filename() && !_File.is_open());

		FileHeader hdr;
		const auto out = ReadOnly ? 0 : ios::out;
		_File.open(Filename, ios::in | out | ios::binary);
		return !_File.is_open() || ReadHeader(hdr);
	}

	// Close an open file.
	// Returns true on error.
	bool Close() {
		assert(_File.is_open());
		
		_File.close();
		return _File.fail();
	}

	// Seek to the beginning of the beginning of the file.
	// Returns true on error.
	bool Rewind() {
		assert(_File.is_open());

		_File.clear();
		_File.seekg(sizeof FileHeader);
		return _File.fail();
	}

	// Seek to the next block.
	// Returns true on error.
	bool Step() {
		assert(_File.is_open());
		
		BlockHeader hdr;
		if (ReadHeader(hdr))
			return true;
		
		_File.seekg(hdr.Size, ios_base::cur);
		return _File.fail();
	}

	// Seek to the next block bearing a particular identity tag.
	// Returns true on error.
	bool Seek(const uint16 Ident) {
		assert(_File.is_open());
		
		// Read blocks until a matching identity tag is found.
		for (BlockHeader hdr;;) {
			// Remember where we are now. This could be it.
			// Read and validate the block header.
			const auto pos = _File.tellg();
			if (_File.fail() || ReadHeader(hdr))
				return true;

			// Does the identity tag match?
			if (hdr.Ident == Ident) {
				// This is the block.
				_File.seekg(pos);
				return false;
			}

			// Seek to the next block.
			_File.seekg(pos + streamoff(hdr.Size));
			if (_File.fail()) {
				// If the seek fails, this is the end.
				_File.seekg(pos);
				_File.clear();
				return true;
			}
		}
	}

	// Seek to the end of the file.
	// Returns true on error.
	bool SeekTail() {
		assert(_File.is_open());
		
		// To find the end, we must start from the beginning.
		if (Rewind())
			return true;

		// Read all blocks until the end is reached.
		for (BlockHeader hdr;;) {
			// Remember where we are now. This could be the end.
			const auto pos = _File.tellg();
			if (_File.fail())
				return true;

			// Read and validate this block header.
			if (ReadHeader(hdr)) {
				// If the read failed, this is the end.
				_File.seekg(pos);
				_File.clear();
				return false;
			}

			// Seek to the next block.
			_File.seekg(pos + streamoff(hdr.Size));
			if (_File.fail() || _File.eof()) {
				// If the seek failed, this is the end.
				_File.seekg(pos);
				_File.clear();
				return false;
			}
		}
	}

	// Write an object to file.
	// Returns true on error.
	template <typename ObjectType>
	bool Write(const ObjectType& Object) {
		assert(_File.is_open());

		_File.write((const char*)&Object, sizeof Object);
		return _File.bad();
	}

	// Write a sequence of objects to file.
	// Returns true on error.
	template <typename DataType>
	bool Write(const DataType* const Storage, const size_t Count) {
		const auto bytes = sizeof DataType * Count;

		assert(bytes < (1ULL << 32));
		assert(_File.is_open());
		
		_File.write((const char*)Storage, bytes);
		return _File.bad();
	}

	// Read an object from file.
	// Returns true on error.
	template <typename ObjectType>
	bool Read(ObjectType& Object) {
		assert(_File.is_open());
		
		_File.read((char*)&Object, sizeof Object);
		return _File.fail();
	}

	// Read a sequence of objects from file.
	// Returns true on error.
	template <typename DataType>
	bool Read(DataType* Storage, const size_t Count) {
		const auto bytes = sizeof DataType * Count;
		
		assert(bytes < (1ULL << 32));
		assert(_File.is_open());

		_File.read((char*)Storage, bytes);
		return _File.fail();
	}

	// Write a block header.
	// Returns true on error.
	template <typename HeaderType>
	bool WriteHeader(HeaderType&& Header) {
		return Write(forward<HeaderType>(Header));
	}

	// Read a block header and validate it.
	// Returns true on error.
	template <typename HeaderType>
	bool ReadHeader(HeaderType& Header) {
		return Read(Header) || Header.Validate();
	}
};


// Expose the basic block header (when a user header is not required).
using BlockHeader = DataStream::BlockHeader;