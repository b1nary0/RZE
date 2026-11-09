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
	// With createIfNotExist, makes the file's folder if the file isn't there yet
	File(const Filepath& filePath, bool createIfNotExist = false);

	// Where to write a file first, so it can be put in place with MoveReplacing once it's whole: path with ".tmp" on
	static Filepath GetTemporaryPathFor(const Filepath& path);
	// Moves the file at from over to, replacing it if it exists, in one step, so anything reading to gets the old file
	// or the new one, never one half-written. False if it couldn't (e.g. while to is open elsewhere), and from is
	// removed either way.
	static bool MoveReplacing(const Filepath& from, const Filepath& to);
	// Moves or renames a file or folder, unless something is already at to. A name that only changes case is allowed.
	// False if it couldn't, and nothing is moved then.
	static bool Move(const Filepath& from, const Filepath& to);

	// If setting file path manually, Read() must be called.
	void SetFilePath(const std::string& path);
	const Filepath& GetPath() const;

	// Open, Read and the writes return false when they fail, and the caller reports it, with what it was doing: File
	// doesn't log, since the log file is written through it, and some callers expect a file to be missing (Config)
	bool Open(EFileOpenMode::Value fileOpenMode);
	void Close();
	bool Read();
	// Empties the file
	bool Overwrite();

	// Ensures the file meets standards of a file
	bool IsValid() const;
	bool Empty() const;

	// Replaces the file's content with data
	template <typename T>
	bool Overwrite(const T& data)
	{
		if (!Open(EFileOpenMode::Write))
		{
			return false;
		}

		m_fileStream << data;
		Close();
		return true;
	}

	// Writes data where the file is (its end, when opened to Append); false if the file isn't open
	template <typename T>
	bool Write(const T& data)
	{
		if (!m_isOpen)
		{
			return false;
		}

		m_fileStream << data;
		return true;
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