#pragma once

#include <EngineCore/Resources/TypedResourceHandle.h>

#include <Rendering/BufferHandle.h>

#include <initializer_list>

class PixelShader;

// One full-screen pass: the quad drawn with shader into whichever colour target is bound, with inputs bound to t0, t1,
// ... in order, and unbound afterwards so the next pass can write to them. The vertex shader (Vertex_RenderTargetQuad),
// topology, target, viewport and constant buffers are the caller's to set, as they differ between passes.
void DrawFullScreenPass(const TypedResourceHandle<PixelShader>& shader, std::initializer_list<Rendering::TextureBuffer2DHandle> inputs);
