#pragma once

#include <Rendering/BufferHandle.h>

#include <Utils/PrimitiveDefs.h>

#include <memory>

class AsyncOperation;
class Filepath;

// Writes textures to image files, through stb_image_write
namespace TextureFile
{
	// Encodes width x height RGBA8 texels (rows from the top, tightly packed) as a PNG, creating the file's folder
	// if it doesn't exist. It's written beside the file and then moved over it (File::MoveReplacing), so what's there
	// is always a whole image. Safe from any thread. False if the file couldn't be written.
	bool WritePNG(const Filepath& filepath, U32 width, U32 height, const U8* rgba);

	// Saves an RGBA8 texture's most detailed mip as an opaque PNG, as drawn by the render commands queued so far:
	// it's read back on the render thread, then encoded and written on a worker. The returned operation is already
	// started, in the background and hidden from progress displays, and succeeds once the file is written; give it a
	// completion callback this frame.
	std::shared_ptr<AsyncOperation> SaveToPNG(const Rendering::TextureBuffer2DHandle& texture, const Filepath& filepath);
}
