#pragma once

namespace Rendering
{
	// Which graphics API the renderer draws with, for code outside Rendering that has to prepare for it (e.g. the window,
	// which SDL makes differently for OpenGL)
	enum class EGraphicsAPI
	{
		DX11,
		OpenGL
	};
}
