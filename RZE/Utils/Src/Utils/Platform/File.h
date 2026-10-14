#pragma once

#include <Utils/Platform/Filepath.h>

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
	// #TODO
	// Hacky constructor to read from non-runtime asset paths
	// until the Filepath infrastructure understands different directory
	// contexts.
	File(const Filepath& filePath, bool createIfNotExist = false);

	// Where to write a file first, so it can be put in place with MoveReplacing once it's whole: path with ".tmp" on
	static Filepath GetTemporaryPathFor(const Filepath& path);
	// Moves the file at from over to, replacing it if it exists, in one step, so anything reading to gets the old file
	// or the new one, never one half-written. False if it couldn't (e.g. while to is open elsewhere), and from is
	// removed either way.
	static bool MoveReplacing(const Filepath& from, const Filepath& to);

	// If setting file path manually, Read() must be called.
	void SetFilePath(const std::string& path);
	const Filepath& GetPath() const;

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
			if (m_isOpen)
			{
				Close();
			}

			Open(EFileOpenMode::Write);

			if (m_isOpen)
			{
				m_fileStream << data;
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
			if (m_isOpen)
			{
				m_fileStream << data;
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

	EFileOpenMode::Value m_openState = EFileOpenMode::Closed;
	std::ofstream m_fileStream;

	Filepath m_filePath;
	std::string m_data;

	bool m_isOpen = false;
};