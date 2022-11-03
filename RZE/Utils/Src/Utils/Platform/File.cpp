#include <Utils/StdAfx.h>
#include <Utils/Platform/File.h>

#include <Utils/DebugUtils/Debug.h>

File::File(const Filepath& filePath, bool createIfNotExist)
	: m_filePath(filePath)
{
	if (createIfNotExist && !filePath.Exists())
	{
		Filepath::CreateDir(filePath.GetAbsoluteDirectoryPath());
	}
}

void File::SetFilePath(const std::string& path)
{
	m_filePath = Filepath(path);
}

const Filepath& File::GetPath() const
{
	return m_filePath;
}

bool File::Open(EFileOpenMode::Value fileOpenMode)
{
	if (m_isOpen)
	{
		Close();
	}

	if (IsValid())
	{
		m_fileStream.open(m_filePath.GetAbsolutePath().c_str(), fileOpenMode);
		
		m_isOpen = m_fileStream.is_open();
		if (!m_isOpen)
		{
			// #TODO Log error
			//RZE_LOG_ARGS("File with path [%s] failed to open.", m_filePath.GetRelativePath().c_str());
			return false;
		}

		m_openState = fileOpenMode;
	}

	return m_isOpen;
}

void File::Close()
{
	if (m_isOpen)
	{
		m_fileStream.close();
		m_isOpen = m_fileStream.is_open();

		if (!m_isOpen)
		{
			m_openState = EFileOpenMode::Value::Closed;
		}
	}
}

bool File::Read()
{
	if (IsValid())
	{
		m_data.clear();

		Open(EFileOpenMode::Value::Read);
		if (m_isOpen)
		{
			std::stringstream stringStream;
			stringStream << m_fileStream.rdbuf();
			m_data = stringStream.str();
			Close();
		}
		else
		{
			// #TODO Log Error
		}
	}

	return m_isOpen;
}

bool File::IsValid() const
{
	// #FIXME - We should probably do some more checks on the string itself to 
	//			ensure that it's an actual filepath, but for now this is fine.
	return m_filePath.IsValid() /*&& !Empty()*/;
}

bool File::Empty() const
{
	return m_data.empty();
}

bool File::Overwrite()
{
	bool bSuccess = false;

	if (IsValid())
	{
		if (m_isOpen)
		{
			Close();
		}

		bSuccess = Open(EFileOpenMode::Write);

		if (bSuccess)
		{
			Close();
		}
	}

	return bSuccess;
}

const std::string& File::Content()
{
	return m_data;
}
