#include <Utils/StdAfx.h>

#include <Utils/DebugUtils/Debug.h>
#include <Utils/Platform/Filepath.h>

#include <filesystem>
#include <Windows.h>

// #TODO
// Ideally this isn't on the Filepath but instead part of the broader
// system. For now though it's the most relevant place.
EDirectoryContext gDirectoryContext = EDirectoryContext::Runtime;

Filepath::Filepath(const std::string& path)
{
	// #TODO
	// This function should actually be a lot simpler, but I'd rather not 
	// touch it until we have the concept of build structure (then all dev can be
	// from the RZE/Assets folder instead of where the executable is).

	std::string pathCpy = path;
	std::replace(pathCpy.begin(), pathCpy.end(), '/', '\\');

	char buffer[1024];
	GetModuleFileNameA(NULL, buffer, 1024);
	std::string execPath(buffer);
	std::replace(execPath.begin(), execPath.end(), '/', '\\');

	if (gDirectoryContext == EDirectoryContext::Tools)
	{
		size_t pos = execPath.find("_Build\\");
		if (pos == std::string::npos)
		{
			// #TODO
			// hacky code to support ci artifacts which wont have the _Build folder.
			// eventually will write good code here but im lazy and bad so who knows
			pos = execPath.find_last_of('\\');
			std::string newpath = execPath.substr(0, pos + 1);
			
			m_absolutePath = newpath + pathCpy;
			m_relativePath = pathCpy;
		}
		else
		{
			std::string newpath = execPath.substr(0, pos);

			m_absolutePath = newpath + pathCpy;
			m_relativePath = pathCpy;
		}
	}
	else
	{
		size_t pos = execPath.find_last_of('\\');
		std::string newpath = execPath.substr(0, pos + 1);

		m_absolutePath = newpath + pathCpy;
		m_relativePath = pathCpy;
	}
}

Filepath::~Filepath()
{
}

void Filepath::CreateDir(const std::string& directoryPath)
{
	std::filesystem::create_directories(directoryPath);
}

bool Filepath::Exists() const
{
	return std::filesystem::exists(GetAbsolutePath());
}

Filepath Filepath::FromAbsolutePathStr(const std::string& absolutePath)
{
	// @TODO having to distinguish between Assets folder and ProjectData needs to be reworked
	// potentially leveraging the DirectoryContext enum
	size_t index = absolutePath.find("Assets\\");
	index = (index != std::string::npos) ? index : absolutePath.find("ProjectData\\");
	if (index != std::string::npos)
	{
		return Filepath(absolutePath.substr(index, absolutePath.size()));
	}

	return Filepath();
}

const std::string& Filepath::GetAbsolutePath() const
{
	return m_absolutePath;
}

const std::string& Filepath::GetRelativePath() const
{
	// #TODO(Josh) for now until we develop this class better
	return m_relativePath;
}

const std::string Filepath::GetAbsoluteDirectoryPath() const
{
	size_t substrCount = m_absolutePath.find_last_of('/');
	return m_absolutePath.substr(0, substrCount + 1);
}

const std::string Filepath::GetRelativeDirectoryPath() const
{
	size_t substrCount = m_relativePath.find_last_of('/');
	return m_relativePath.substr(0, substrCount + 1);
}

bool Filepath::IsValid() const
{
	// #TODO(This is an absolutely stupid but still relevant condition)
	return !m_absolutePath.empty();
}

void Filepath::SetDirectoryContext(EDirectoryContext context)
{
	gDirectoryContext = context;
}

void Filepath::ConvertToWindowsFormat()
{
	std::replace(m_absolutePath.begin(), m_absolutePath.end(), '/', '\\');
	std::replace(m_relativePath.begin(), m_relativePath.end(), '/', '\\');
}
