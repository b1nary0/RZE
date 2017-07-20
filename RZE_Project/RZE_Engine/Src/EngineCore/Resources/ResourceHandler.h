#pragma once

#include <unordered_map>

#include <EngineCore/Resources/Resource.h>

class ResourceHandle
{
    friend class ResourceHandler;

public:
    ResourceHandle();

    bool IsValid();
    const std::string& GetID() const;

    bool operator==(const ResourceHandle& rhs)
    {
        rhs.mResourceID == mResourceID;
    }

private:
    ResourceHandle(const std::string& resourceID);

    std::string mResourceID;
};

class ResourceHandler
{
public:
    ResourceHandler();
    ~ResourceHandler();

    ResourceHandle RequestResource(const std::string& resourcePath);

private:
    template <class ResourceT>
    IResource* CreateAndLoadResource();

    std::unordered_map<std::string, IResource*> mResourceTable;
};