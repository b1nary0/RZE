#pragma once

#include <Utils/Reflect/ReflectDB.h>

#include <functional>

#define REFLECT_REGISTER_COMPONENT(ComponentType) \
{ \
ReflectDB::RegisterComponent(REFLECT_NAME(ComponentType)); \
} \

#define REFLECT_REGISTER_COMPONENT_CHILD(ComponentType, ParentType) \
{ \
ReflectDB::RegisterComponentChild(REFLECT_NAME(ComponentType), REFLECT_NAME(ParentType)); \
} \

#define REFLECT_NAME(ComponentType) #ComponentType