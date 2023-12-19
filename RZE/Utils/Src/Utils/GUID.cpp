#include <StdAfx.h>
#include <Utils/GUID.h>

#include <Windows.h>

namespace GUIDHelper
{
	GUID GUIDHelper::GenerateGUID()
	{
		GUID gidRef;
		HRESULT hCreateGuid = CoCreateGuid(&gidRef);

		return gidRef;
	}
}
