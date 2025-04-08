#include <StdAfx.h>

#include <Utils/Reflect/ReflectDB.h>

ReflectDB::ReflectRegistry ReflectDB::m_registry;

void ReflectDB::RegisterComponent(const char* typeStr)
{
	ReflectTypeDescriptor typeDescriptor;

	typeDescriptor.typeID = ReflectDB::GenerateTypeID(typeStr);
	typeDescriptor.name = typeStr;

	m_registry[typeDescriptor.typeID] = typeDescriptor;
}

void ReflectDB::RegisterComponentChild(const char* typeStr, const char* parentTypeStr)
{
	RegisterComponent(typeStr);
	ReflectTypeID typeID = GetIDFromName(typeStr);
	ReflectTypeID parentTypeID = GetIDFromName(parentTypeStr);

	ReflectTypeDescriptor& parentTypeDescriptor = m_registry[parentTypeID];
	parentTypeDescriptor.children.push_back(typeID);

	ReflectTypeDescriptor& childTypeDescriptor = m_registry[typeID];
	childTypeDescriptor.parentTypeID = parentTypeID;
}

size_t ReflectDB::GetIDFromName(const char* typeName)
{
	// #TODO: Measure hashing speed to see if we should look for other methods
	ReflectTypeID typeID = ReflectDB::GenerateTypeID(typeName);
	return typeID;
}

size_t ReflectDB::GenerateTypeID(const char* typeStr)
{
	std::hash<const char*> hasher;
	return hasher(typeStr);
}
