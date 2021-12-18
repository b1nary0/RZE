#pragma once

#include <Utils/PrimitiveDefs.h>
#include <Utils/Platform/FilePath.h>

class ByteStream
{
public:
	ByteStream(const std::string& name);
	ByteStream(const std::string& name, size_t streamLength);
	~ByteStream();

public:
	inline const unsigned char* GetByteStream() const { return mBytes; }
	unsigned char* GetByteStream() { return mBytes; }

	void ReadFromFile(const FilePath& filePath);

	// #TODO
	// WriteToStream/FlushToFile type functions to flesh out this class.
	// Then the File class can essentially just hold one of these 

	template <typename T>
	T* ReadBytes()
	{
		T* byteCast = static_cast<T*>(mBytes);
		curPos += sizeof(T);

		return byteCast;
	}

	bool WriteBytes(unsigned char* buf, size_t writeLength);

private:
	std::string mName;
	Byte* mBytes = nullptr;

	size_t curPos = 0; // Byte "cursor" - index of byte next to be read
};