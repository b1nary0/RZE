#include <StdAfx.h>
#include <EngineCore/Platform/File.h>

File::File(const std::string& filePath)
	: mOpenState(EFileOpenMode::Closed)
	, bIsOpen(false)
{
	mFilePath = filePath;

	Read();
}

void File::SetFilePath(const std::string& path)
{
	mFilePath = path;
}

bool File::Open(EFileOpenMode::Value fileOpenMode)
{
	if (bIsOpen)
	{
		Close();
	}

	if (IsValid())
	{
		mFileStream.open(mFilePath.c_str(), fileOpenMode);

		bIsOpen = mFileStream.is_open();
		if (!bIsOpen)
		{
			// #TODO Log error
			LOG_CONSOLE_ARGS("File with path [%s] failed to open.", mFilePath.c_str());
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
	bool bIsValid = !mFilePath.empty();

	return bIsValid;
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
