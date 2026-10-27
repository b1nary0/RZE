#include <StdAfx.h>
#include <Utils/LiveObjectPtr.h>

#include <Utils/DebugUtils/Debug.h>

namespace LiveObjectPtrDetail
{
	void CheckAccess([[maybe_unused]] bool isAlive)
	{
		AssertMsg(isAlive, "Handle used after its object was destroyed");
	}
}
