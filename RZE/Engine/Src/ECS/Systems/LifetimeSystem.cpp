#include <StdAfx.h>

//void LifetimeSystem::Initialize()
//{
//	InternalGetComponentFilter().AddFilterType<LifetimeComponent>();
//}
//
//void LifetimeSystem::Update(const std::vector<Apollo::EntityID>& entities)
//{
//	OPTICK_EVENT();
//
//	for (Apollo::EntityID entity : entities)
//	{
//		LifetimeComponent* const component = InternalGetEntityHandler().GetComponent<LifetimeComponent>(entity);
//		component->LifetimeMS -= static_cast<float>(RZE_Application::RZE().GetDeltaTime() * 1000);
//
//		if (component->LifetimeMS <= 0)
//		{
//			InternalGetEntityHandler().DestroyEntity(entity);
//		}
//	}
//}
//
//void LifetimeSystem::ShutDown()
//{
//
//}
