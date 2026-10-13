#include <StdAfx.h>
#include <Graphics/TextureFile.h>

#include <EngineCore/Async/AsyncOperation.h>
#include <EngineCore/Async/AsyncOperationManager.h>

#include <Rendering/Renderer.h>
#include <Rendering/TextureReadback.h>

#include <Utils/Platform/Filepath.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <STB/stb_image_write.h>

namespace
{
	constexpr U32 k_rgbaBytesPerTexel = 4;

	// Waits for the readback, then writes the file on a worker
	class SaveTextureOperation final : public AsyncOperation
	{
	public:
		SaveTextureOperation(const Rendering::TextureBuffer2DHandle& texture, const Filepath& filepath)
			: AsyncOperation("Saving " + filepath.GetRelativePath(), EAsyncOperationFlags::None)
			, m_filepath(filepath)
			, m_readback(std::make_shared<Rendering::TextureReadback>())
		{
			// Queued now rather than in OnStart (next frame), so it reads what's been drawn so far and not what a
			// later frame draws over it
			Rendering::Renderer::ReadTexture(texture, m_readback);
		}

	protected:
		void OnStart() override {}

		void OnTick(AsyncFrameBudget& budget) override
		{
			if (!m_readback->IsReady.load(std::memory_order_acquire))
			{
				return;
			}

			if (!m_writeJob.IsValid())
			{
				if (m_readback->BytesPerTexel != k_rgbaBytesPerTexel)
				{
					Fail("Only RGBA8 textures can be saved as PNG");
					return;
				}

				m_writeJob = PushWorkerJob(Threading::Job::Task([this]()
					{
						Rendering::TextureReadback& readback = *m_readback;

						// What a render target holds in alpha isn't meant to be seen
						for (size_t alpha = 3; alpha < readback.Data.size(); alpha += k_rgbaBytesPerTexel)
						{
							readback.Data[alpha] = 255;
						}

						m_wasWritten = TextureFile::WritePNG(m_filepath, readback.Width, readback.Height, readback.Data.data());
					}));
				return;
			}

			if (!m_writeJob.IsComplete())
			{
				return;
			}

			if (m_wasWritten)
			{
				Complete();
			}
			else
			{
				Fail("Couldn't write " + m_filepath.GetRelativePath());
			}
		}

	private:
		Filepath m_filepath;
		std::shared_ptr<Rendering::TextureReadback> m_readback;
		Threading::JobHandle m_writeJob;
		// Written by the worker, read on the main thread once m_writeJob is complete
		bool m_wasWritten = false;
	};
}

namespace TextureFile
{
	bool WritePNG(const Filepath& filepath, U32 width, U32 height, const U8* rgba)
	{
		Filepath::CreateDir(filepath.GetAbsoluteDirectoryPath());

		const int stride = static_cast<int>(width * k_rgbaBytesPerTexel);
		return stbi_write_png(filepath.GetAbsolutePath().c_str(), static_cast<int>(width), static_cast<int>(height),
			static_cast<int>(k_rgbaBytesPerTexel), rgba, stride) != 0;
	}

	std::shared_ptr<AsyncOperation> SaveToPNG(const Rendering::TextureBuffer2DHandle& texture, const Filepath& filepath)
	{
		std::shared_ptr<AsyncOperation> operation = std::make_shared<SaveTextureOperation>(texture, filepath);
		RZE().GetAsyncOperationManager().Start(operation);
		return operation;
	}
}
