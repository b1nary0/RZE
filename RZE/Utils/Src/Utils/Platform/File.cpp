#include <Utils/StdAfx.h>
#include <Utils/Platform/File.h>

#include <Utils/DebugUtils/Debug.h>

File::File(const std::string& filePath)
	: mOpenState(EFileOpenMode::Closed)
	, bIsOpen(false)
{
	// #TODO
	// Standardize FilePath in the API
	mFilePath = FilePath(filePath);

	Read();
}

File::File(const FilePath& filePath)
{
	mFilePath = filePath;
	Read();
}

void File::SetFilePath(const std::string& path)
{
	mFilePath = FilePath(path);
}

const FilePath& File::GetPath() const
{
	return mFilePath;
}

bool File::Open(EFileOpenMode::Value fileOpenMode)
{
	if (bIsOpen)
	{
		Close();
	}

	if (IsValid())
	{
		mFileStream.open(mFilePath.GetAbsolutePath().c_str(), fileOpenMode);

		bIsOpen = mFileStream.is_open();
		if (!bIsOpen)
		{
			// #TODO Log error
			LOG_CONSOLE_ARGS("File with path [%s] failed to open.", mFilePath.GetRelativePath().c_str());
			return false;
		}

		mOpenState = fileOpenMode;
	}

	return bIsOpen;
}

void File::Close()
{
	if (bIsOpen)
	{
		mFileStream.close();
		bIsOpen = mFileStream.is_open();

		if (!bIsOpen)
		{
			mOpenState = EFileOpenMode::Value::Closed;
		}
	}
}

bool File::Read()
{
	if (IsValid())
	{
		mData.clear();

		Open(EFileOpenMode::Value::Read);
		if (bIsOpen)
		{
			std::stringstream stringStream;
			stringStream << mFileStream.rdbuf();
			mData = stringStream.str();
			Close();
		}
		else
		{
			// #TODO Log Error
		}
	}

	return bIsOpen;
}

bool File::IsValid() const
{
	// #FIXME - We should probably do some more checks on the string itself to 
	//			ensure that it's an actual filepath, but for now this is fine.
	return mFilePath.IsValid() /*&& !Empty()*/;
}

bool File::Empty() const
{
	return mData.empty();
}

bool File::Overwrite()
{
	bool bSuccess = false;

	if (IsValid())
	{
		if (bIsOpen)
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
	return mData;
}
