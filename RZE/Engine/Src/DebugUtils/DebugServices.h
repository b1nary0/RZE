#pragma once

#include <string>
#include <deque>

#include <Utils/Math/Vector3D.h>

class DebugServices
{
private:
	struct LogEntry
	{
		Vector3D TextColor;
		std::string Text;
	};

public:
	DebugServices();

	static void AddLog(const std::string& text, const Vector3D& color = Vector3D(1.0f, 1.0f, 0.0f));
	static void AddData(const std::string& text, const Vector3D& color = Vector3D(1.0f, 1.0f, 0.0f));

	static void Initialize();
	static void Display(const Vector2D& windowSize);

	static void HandleScreenResize(const Vector2D& windowSize);

private:
	static void RenderData(const Vector2D& windowSize);

private:
	static std::vector<LogEntry> mDataEntries;

	static RZE_Editor* sEditor;
};