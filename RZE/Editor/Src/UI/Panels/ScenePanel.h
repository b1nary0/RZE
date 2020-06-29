#pragma once

#include <Utils/PrimitiveDefs.h>

#include <string>
#include <vector>

namespace Editor
{
	class ScenePanel
	{
		struct EntityItem
		{
			U32 EntityID;
		};

	public:
		ScenePanel();
		~ScenePanel();

	public:
		void Display();

		void Enable();
		void Disable();
		bool IsEnabled();

		bool HasSelectedEntity();
		U32 GetSelectedEntityID();

	private:
		bool bEnabled;

		std::vector<std::string> mSelectedComponents;

		EntityItem* mSelectedItem;
	};
}