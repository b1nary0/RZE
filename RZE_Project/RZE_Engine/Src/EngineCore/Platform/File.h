#pragma once

#include <string>
#include <fstream>

class File
{
public:

	struct EFileOpenMode
	{
		typedef int Type; // std::ios_base::openmode

		enum Value : Type
		{
			Append = std::ios_base::app,
			AtEnd = std::ios_base::ate,
			Binary = std::ios_base::binary,
			Read = std::ios_base::in,
			Write = std::ios_base::out,
			Truncate = std::ios_base::trunc,
			Closed = 0x800
		};
	};

public:
	File() = default;
	File(const std::string& filePath);

	// If setting file path manually, Read() must be called.
	void SetFilePath(const std::string& path);

	bool Open(EFileOpenMode::Value fileOpenMode);
	void Close();
	bool Read();
	bool Overwrite();

	// Ensures the file meets standards of a file
	bool IsValid() const;
	bool Empty() const;

	// Overwrites with content of Data
	template <typename T>
	void Overwrite(const T& data)
	{
		if (IsValid())
		{
			if (bIsOpen)
			{
				Close();
			}

			Open(EFileOpenMode::Write);

			if (bIsOpen)
			{
				mFileStream << data;
				Close();
			}
			else
			{
				// #TODO Log Error
			}
		}
	}

	// Check for if the file has nothing in it, or didn't load properly

	// Appends
	template <typename T>
	void Write(const T& data)
	{
		if (IsValid())
		{
			if (bIsOpen)
			{
				mFileStream << data;
			}
			else
			{
				// Error
			}
		}
	}

	const std::string& Content();

	template <typename T>
	inline void operator<<(const T& data)
	{
		if (IsValid())
		{
			Write(data);
		}
	}
	
private:

	EFileOpenMode::Value mOpenState;

	std::ofstream mFileStream;

	std::string mFilePath;

	std::string mData;

	bool bIsOpen;

};