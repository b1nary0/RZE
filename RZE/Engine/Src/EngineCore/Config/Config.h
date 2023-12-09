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
	void Read(const std::string& filePath);

	File mINIFile;
	CSimpleIniA mINIParser;
};