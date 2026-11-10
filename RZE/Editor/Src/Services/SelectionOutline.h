#pragma once

class RenderView;

namespace Editor
{
	class EditorSelection;

	// Outlines the selected GameObjects in view's next render, each with its descendants: adds one RenderOutline around
	// their meshes, faint where they're behind the scene (OutlineRenderStage). Each selected object and its descendants
	// make a shape of their own, numbered in selection order, so each is edged even inside or in front of another (a
	// selected object under a selected ancestor is in the ancestor's). Adds nothing with nothing selected. Called every
	// frame, as the view's requests are cleared after each render (RenderView::ClearFrameRequests) and the render objects
	// to outline change without the selection changing: children and components come and go, and an object's render
	// object is made again when it comes back into the scene (an undone delete).
	void ApplySelectionOutline(const EditorSelection& selection, RenderView& view);
}
