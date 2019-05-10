#pragma once

namespace Editor
{
	class ScenePanel
	{
	public:
		ScenePanel();
		~ScenePanel();

	public:
		void Display();

		void Enable();
		void Disable();
		bool IsEnabled();

	private:
		bool bEnabled;
	};
}