#pragma once

#include <SimpleINI/SimpleIni.h>

#include <Utils/Platform/File.h>

class FilePath;

class Config
{
public:
	Config();
	~Config();

	virtual void Load(const FilePath& filePath) = 0;

	bool Empty();

protected:
	void Read(const std::string& filePath);

	File mINIFile;
	CSimpleIniA mINIParser;
};