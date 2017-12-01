#pragma once

#include <Utils/Interfaces/SubSystem.h>

namespace Apollo
{
	class EntityComponentSystem : public ISubSystem
	{
	public:

		virtual void Initialize();
		virtual void Update();
		virtual void ShutDown();
	};
}