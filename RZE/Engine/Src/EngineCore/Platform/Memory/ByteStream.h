#pragma once

#include <Utils/PrimitiveDefs.h>
#include <Utils/Platform/FilePath.h>

class ByteStream
{
public:
	ByteStream(const std::string& name);
	~ByteStream();

public:
	inline const unsigned char* GetByteStream() const { return mBytes; }
	unsigned char* GetByteStream() { return mBytes; }

	void ReadFromFile(const FilePath& filePath);

	// #TODO
	// WriteToStream/FlushToFile type functions to flesh out this class.
	// Then the File class can essentially just hold one of these 

private:
	std::string mName;
	Byte* mBytes = nullptr;
};