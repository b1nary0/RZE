#pragma once

class IResource
{
public:
    virtual bool Load(const std::string& filePath) = 0;

protected:
};
