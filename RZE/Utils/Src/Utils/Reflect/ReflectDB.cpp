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

	ReflectTypeID typeID = GenerateTypeID(typeStr);
	ReflectTypeID parentTypeID = GenerateTypeID(parentTypeStr);

	ReflectTypeDescriptor& parentTypeDescriptor = m_registry[parentTypeID];
	parentTypeDescriptor.children.push_back(typeID);

	ReflectTypeDescriptor& childTypeDescriptor = m_registry[typeID];
	childTypeDescriptor.parentTypeID = parentTypeID;
}

size_t ReflectDB::GenerateTypeID(const char* typeStr)
{
	std::hash<const char*> hasher;
	return hasher(typeStr);
}
