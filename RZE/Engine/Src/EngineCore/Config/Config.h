#pragma once

#include <SimpleINI/SimpleIni.h>

#include <Utils/Interfaces/Resource.h>
#include <Utils/Platform/File.h>

class Filepath;

class Config : public IResource
{
public:
	Config();
	virtual ~Config();

	bool Empty() const;

protected:
	void Read(const Filepath& filePath);
	// Saves the parser's values, with the comments Read found, over filePath. False if it couldn't.
	bool Write(const Filepath& filePath) const;

	File mINIFile;
	CSimpleIniA mINIParser;
};