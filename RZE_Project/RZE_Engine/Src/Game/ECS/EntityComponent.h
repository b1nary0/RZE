#pragma once

#include <string>

class IEntityComponent
{
public:
    inline const std::string& GetName() const { return mName; }
    inline void SetName(const std::string& newName) { mName = newName; }

private:
    std::string mName;
};
