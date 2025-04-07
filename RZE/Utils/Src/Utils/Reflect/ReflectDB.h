#pragma once

#include <Utils/PrimitiveDefs.h>

#include <Utils/Reflect/Reflection.h>

#include <array>
#include <string.h>
#include <vector>
#include <unordered_map>

typedef U32 ReflectTypeID;

class ReflectDB
{
public:
	static void RegisterComponent(const char* typeStr);

private:
	struct ReflectTypeDescriptor
	{
		std::string name;
		ReflectTypeID id = 0;

		std::vector<ReflectTypeID> m_children;
	};

	typedef std::unordered_map<ReflectTypeID, ReflectTypeDescriptor> ReflectRegistry;

	static size_t GenerateTypeID(const char* typeStr);

private:
	static ReflectRegistry m_registry;

	static ReflectTypeID k_reflectKey;
};
