#pragma once

#include <deque>
#include <string>

#include <Editor/Widgets/EditorWidget.h>

#include <Utils/Math/Vector3D.h>

class LogWidget : public IEditorWidget
{
private:
	struct LogEntry
	{
		Vector3D TextColor;
		std::string Text;
	};

public:
	LogWidget() = default;
	virtual ~LogWidget() = default;

	// IEditorWidget interface
public:
	virtual void Display() override;

public:
	void AddEntry(const std::string& text, const Vector3D& color = mDefaultLogColor);

private:
	static Vector3D mDefaultLogColor;

	std::deque<LogEntry> mLogEntries;
};