#pragma once

#include <vector>

#include <Apollo/ECS/EntityAdmin.h>

#include <Utils/Interfaces/SubSystem.h>

namespace Apollo
{
	class IEntity;

	class EntityComponentSystem : public ISubSystem
	{
		typedef std::vector<IEntity*> EntityList;

		class EntityAdminImpl : public IEntityAdmin
		{
		// IEntityAdmin interface
		public:
			virtual void Initialize();
			virtual void Update();
			virtual void ShutDown();
		};

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
			return mAdmin.AddEntity<TEntityType>(std::forward<TArgs>(args)...);
		}

		template <class TSystem, typename... TArgs>
		void AddSystem(TArgs... args)
		{
			mAdmin.AddSystem<TSystem>(std::forward<TArgs>(args)...);
		}

	private:
		EntityAdminImpl mAdmin;
	};
}