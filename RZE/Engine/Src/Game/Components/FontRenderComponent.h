#pragma once

#include <Game/ECS/EntityComponent.h>

class FontRenderComponent : public IEntityComponent
{
public:
	FontRenderComponent();
	FontRenderComponent(const std::string& name);
	virtual ~FontRenderComponent();

	void SetFont(const ResourceHandle& fontResource);
	void SetText(const std::string& text);

	ResourceHandle GetFont();
	const std::string& GetText();

private:
	ResourceHandle mFontResource;
	std::string mText;
};