#include <doctest/doctest.h>

#include <Utils/Platform/TypeNames.h>

namespace TypeNamesTestTypes
{
	class SomeClass {};
	struct SomeStruct {};
}

class GlobalClass {};

TEST_CASE("TypeNames::Get gives a type's name as the code spells it, on every compiler")
{
	CHECK(TypeNames::Get(typeid(GlobalClass)) == "GlobalClass");
	CHECK(TypeNames::Get(typeid(TypeNamesTestTypes::SomeClass)) == "TypeNamesTestTypes::SomeClass");
	CHECK(TypeNames::Get(typeid(TypeNamesTestTypes::SomeStruct)) == "TypeNamesTestTypes::SomeStruct");
}
