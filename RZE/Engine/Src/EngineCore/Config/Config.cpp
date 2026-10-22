#include <StdAfx.h>
#include <EngineCore/Config/Config.h>

Config::Config()
{
	// Writes "Key=Value", as the files are written by hand
	mINIParser.SetSpaces(false);
}

Config::~Config()
{
}

bool Config::Empty() const
{
	const bool bFileEmpty = mINIFile.Empty();
	const bool bParserEmpty = mINIParser.IsEmpty();

	return bFileEmpty && bParserEmpty;
}

void Config::Read(const Filepath& filePath)
{
	mINIFile.SetFilePath(filePath.GetRelativePath());
	mINIFile.Read();
	mINIFile.Close();

	if (!mINIFile.Empty())
	{
		mINIParser.LoadData(mINIFile.Content());
	}
	else
	{
		RZE_LOG_WARNING_ARGS("Could not open file at path [%s]", mINIFile.GetPath().GetRelativePath().c_str());
	}
}

bool Config::Write(const Filepath& filePath) const
{
	// Beside it first, then moved over it, so anything reading it meanwhile never gets half a file
	const Filepath temporaryPath = File::GetTemporaryPathFor(filePath);
	const bool wasWritten = mINIParser.SaveFile(temporaryPath.GetAbsolutePath().c_str(), false) == SI_OK;

	return File::MoveReplacing(temporaryPath, filePath) && wasWritten;
}
