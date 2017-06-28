#pragma once

#include <SimpleINI/SimpleIni.h>

#include <EngineCore/Platform/File.h>

class Config
{
public:
    Config();
    ~Config();

    virtual void Load(const std::string& filePath) = 0;

    bool Empty();

protected:
    void Read(const std::string& filePath);

    File mINIFile;
    CSimpleIniA mINIParser;
};