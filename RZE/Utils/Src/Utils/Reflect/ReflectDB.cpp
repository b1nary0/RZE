#include <StdAfx.h>

#include <Utils/Reflect/ReflectDB.h>

ReflectTypeID ReflectDB::k_reflectKey = 0;

void ReflectDB::RegisterComponent(const char* typeStr)
{
	ReflectTypeID typeID = k_reflectKey++;
	ReflectTypeDescriptor typeDescriptor;

	typeDescriptor.id = ReflectDB::GenerateTypeID(typeStr);
	typeDescriptor.name = typeStr;

	m_registry[typeID] = typeDescriptor;
}

size_t ReflectDB::GenerateTypeID(const char* typeStr)
{
	std::hash<const char*> hasher;
	return hasher(typeStr);
}

ReflectDB::ReflectRegistry ReflectDB::m_registry;

