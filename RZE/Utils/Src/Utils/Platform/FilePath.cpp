#include <Utils/StdAfx.h>

#include <Utils/Platform/FilePath.h>

#include <Windows.h>

FilePath::FilePath(const std::string& path)
{
	char buffer[1024];
	GetModuleFileNameA(NULL, buffer, 1024);
	std::string execPath(buffer, 1024);

	std::replace(execPath.begin(), execPath.end(), '\\', '/');
	size_t pos = execPath.find_last_of("\\/");
	std::string newpath = execPath.substr(0, pos + 1);
	
	// #Josh(Temp fix until I get off my ass and make this better)
	mAbsolutePath = newpath + path;
	mRelativePath = path;
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

bool FilePath::IsValid()
{
	return !mAbsolutePath.empty();
}
