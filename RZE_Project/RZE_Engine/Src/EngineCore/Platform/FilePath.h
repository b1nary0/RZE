#pragma once

#include <string>

//
// @implementation: will do this later, leaving as idea.
//

class FilePath
{
public:
    FilePath();
    ~FilePath();

    const std::string& GetAbsolutePath() const;
    const std::string& GetRelativePath() const;

private:
    std::string mAbsolutePath;
};