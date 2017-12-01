#pragma once

#include <vector>

#include <Utils/Interfaces/SubSystem.h>

namespace Apollo
{
	class IEntity;

	class EntityComponentSystem : public ISubSystem
	{
		typedef std::vector<IEntity*> EntityList;

	public:
		EntityComponentSystem();
		virtual ~EntityComponentSystem();

	// ISubSystem interface
	public:
		virtual void Initialize();
		virtual void Update();
		virtual void ShutDown();

	// EntityComponentSystem API
	public:
		template<class TEntityType, typename... TArgs>
		TEntityType* AddEntity(TArgs... args)
		{
			TEntityType* entity = new TEntityType(std::forward<TArgs>(args)...);
			mEntities.push_back(entity);
			return mEntities.back();
		}

	private:
		EntityList mEntities;
	};
}