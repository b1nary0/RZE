#pragma once

#include <SimpleINI/SimpleIni.h>

#include <Utils/Platform/File.h>

class Filepath;

class Config
{
public:
	Config();
	virtual ~Config();

	virtual void Load(const Filepath& filePath) = 0;

	bool Empty();

protected:
	void Read(const std::string& filePath);

	File mINIFile;
	CSimpleIniA mINIParser;
};