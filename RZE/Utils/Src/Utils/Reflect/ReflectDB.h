#pragma once

#include <Utils/PrimitiveDefs.h>

#include <Utils/Reflect/Reflection.h>

#include <array>
#include <string.h>
#include <vector>
#include <unordered_map>

typedef size_t ReflectTypeID;

class ReflectDB
{
public:
	static void RegisterComponent(const char* typeStr);
	static void RegisterComponentChild(const char* typeStr, const char* parentTypeStr);

	static size_t GetIDFromName(const char* typeName);

private:
	struct ReflectTypeDescriptor
	{
		std::string name;
		ReflectTypeID typeID = 0;
		ReflectTypeID parentTypeID = 0;

		std::vector<ReflectTypeID> children;
	};

	typedef std::unordered_map<ReflectTypeID, ReflectTypeDescriptor> ReflectRegistry;

	static size_t GenerateTypeID(const char* typeStr);

private:
	static ReflectRegistry m_registry;
	static ReflectTypeID k_reflectKey;
};
