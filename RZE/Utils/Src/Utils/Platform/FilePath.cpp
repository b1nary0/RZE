#include <Utils/StdAfx.h>

#include <Utils/DebugUtils/Debug.h>
#include <Utils/Platform/FilePath.h>

#include <Windows.h>

// #TODO
// Ideally this isn't on the FilePath but instead part of the broader
// system. For now though it's the most relevant place.
EDirectoryContext gDirectoryContext = EDirectoryContext::Runtime;

FilePath::FilePath(const std::string& path)
{
	char buffer[1024];
	GetModuleFileNameA(NULL, buffer, 1024);
	std::string execPath(buffer);
	std::replace(execPath.begin(), execPath.end(), '\\', '/');

	if (gDirectoryContext == EDirectoryContext::Tools)
	{
		size_t pos = execPath.find("RZE/RZE");
		AssertMsg(pos != std::string::npos, "Directory structure invalid.");

		// #NOTE
		// Magic number 8 is giving us everything to RZE/RZE/ so we can tack on the 
		// relative given path.
		std::string newpath = execPath.substr(0, pos + 8);
		std::string pathCpy = path;
		std::replace(pathCpy.begin(), pathCpy.end(), '\\', '/');
		
		mAbsolutePath = newpath + pathCpy;
		mRelativePath = pathCpy;
	}
	else
	{
		size_t pos = execPath.find_last_of('/');
		std::string newpath = execPath.substr(0, pos + 1);

		// #TODO(This is just for std::replace, but should be dealt with later)
		std::string pathCpy = path;
		std::replace(pathCpy.begin(), pathCpy.end(), '\\', '/');
		mAbsolutePath = newpath + pathCpy;
		mRelativePath = pathCpy;
	}
}

FilePath::FilePath(const std::string& path, const bool isCustomPath)
{
	// #TODO
	// WARNING:
	// This constructor is temporary only, any use other than what it was made for will
	// likely break. Just didn't have time to go down this rabbit hole at the moment.
	mRelativePath = path;
	mAbsolutePath = path;
}

FilePath::~FilePath()
{
}

const std::string& FilePath::GetAbsolutePath() const
{
	return mAbsolutePath;
}

const std::string& FilePath::GetRelativePath() const
{
	// #TODO(Josh) for now until we develop this class better
	return mRelativePath;
}

const std::string FilePath::GetAbsoluteDirectoryPath() const
{
	size_t substrCount = mAbsolutePath.find_last_of('/');
	return mAbsolutePath.substr(0, substrCount + 1);
}

const std::string FilePath::GetRelativeDirectoryPath() const
{
	size_t substrCount = mRelativePath.find_last_of('/');
	return mRelativePath.substr(0, substrCount + 1);
}

bool FilePath::IsValid() const
{
	// #TODO(This is an absolutely stupid but still relevant condition)
	return !mAbsolutePath.empty();
}

void FilePath::SetDirectoryContext(EDirectoryContext context)
{
	gDirectoryContext = context;
}
