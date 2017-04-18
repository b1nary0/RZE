#pragma once

class Win32Window;

class RZE_Renderer
{
public:

	RZE_Renderer();
	~RZE_Renderer();

	void Render();

	std::weak_ptr<Win32Window> MakeWindow(const std::string& title, const int width, const int height);

	std::weak_ptr<Win32Window> GetMainWindow() const;

private:

	std::shared_ptr<Win32Window> mMainWindow;
};