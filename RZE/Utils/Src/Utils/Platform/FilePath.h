#pragma once

#include <string>

//
// @implementation: will do this later, leaving as idea.
//

class FilePath
{
public:
	FilePath() = default;
	explicit FilePath(const std::string& path);
	~FilePath();

	const std::string& GetAbsolutePath() const;
	const std::string& GetRelativePath() const;

private:
	std::string mAbsolutePath;
	std::string mRelativePath;
};