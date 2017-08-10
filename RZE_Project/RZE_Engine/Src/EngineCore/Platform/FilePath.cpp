#include <StdAfx.h>

#include <EngineCore/Platform/FilePath.h>

FilePath::FilePath(const std::string& path)
{
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
    return mAbsolutePath;
}
