//#include <StdAfx.h>
#include <Diotima/Renderer.h>

#include <Diotima/Driver/OpenGL.h>
#include <Diotima/Graphics/Material.h>
#include <Diotima/Graphics/Mesh.h>
#include <Diotima/Graphics/Texture2D.h>
#include <Diotima/Shaders/ShaderPipeline.h>

#include <Utils/Conversions.h>
#include <Utils/DebugUtils/Debug.h>
#include <Utils/Math/Vector4D.h>
#include <Utils/Platform/FilePath.h>

#include <imGUI/imgui.h>


Diotima::GFXShaderPipeline* renderToTextureShader = nullptr;
void CreateRenderToTextureShader()
{
	const FilePath vertShaderFilePath("Engine/Assets/Shaders/RenderToTextureVert.shader");
	const FilePath fragShaderFilePath("Engine/Assets/Shaders/RenderToTextureFrag.shader");

	Diotima::GFXShader* vertShader = new Diotima::GFXShader(EGLShaderType::Vertex, "RenderToTextureVertex");
	vertShader->Load(vertShaderFilePath.GetAbsolutePath());
	vertShader->Create();
	vertShader->Compile();

	Diotima::GFXShader* fragShader = new Diotima::GFXShader(EGLShaderType::Fragment, "RenderToTextureFragment");
	fragShader->Load(fragShaderFilePath.GetAbsolutePath());
	fragShader->Create();
	fragShader->Compile();

	renderToTextureShader = new Diotima::GFXShaderPipeline("TextureShader");
	renderToTextureShader->AddShader(Diotima::GFXShaderPipeline::EShaderIndex::Vertex, vertShader);
	renderToTextureShader->AddShader(Diotima::GFXShaderPipeline::EShaderIndex::Fragment, fragShader);

	renderToTextureShader->GenerateShaderProgram();
}

namespace Diotima
{
	Renderer::Renderer()
	{
	}

	Int32 Renderer::AddRenderItem(const RenderItemProtocol& itemProtocol)
	{
		if (!mFreeRenderListIndices.empty())
		{
			Int32 index = mFreeRenderListIndices.front();
			mFreeRenderListIndices.pop();

			mRenderList[index] = std::move(itemProtocol);
			mRenderList[index].bIsValid = true;

			return index;
		}

		mRenderList.emplace_back(std::move(itemProtocol));
		mRenderList.back().bIsValid = true;

		return static_cast<Int32>(mRenderList.size() - 1);
	}

	void Renderer::RemoveRenderItem(const U32 itemIdx)
	{
		mRenderList[itemIdx].Invalidate();
		mFreeRenderListIndices.push(itemIdx);
	}

	Int32 Renderer::AddLightItem(const LightItemProtocol& itemProtocol)
	{
		mLightingList.emplace_back(std::move(itemProtocol));
		return static_cast<Int32>(mLightingList.size() - 1);
	}

	void Renderer::Initialize()
	{
		{
			OpenGLRHI::OpenGLCreationParams creationParams;
			creationParams.WindowHeight = 1024;
			creationParams.WindowWidth = 768;
			OpenGLRHI::Get().Init(creationParams);
		}

		OpenGLRHI::Get().ClearColor(0.25f, 0.25f, 0.25f, 1.0f);

		OpenGLRHI::Get().EnableCapability(EGLCapability::DepthTest);

		CreateRenderToTextureShader();
	}

	void Renderer::Update()
	{
		const OpenGLRHI& openGL = OpenGLRHI::Get();

		openGL.Clear(EGLBufferBit::Color | EGLBufferBit::Depth);
		for(auto& renderItem : mRenderList)
		{
			if (renderItem.bIsValid)
			{
				RenderSingleItem(renderItem);
			}
		}
	}

	void Renderer::ShutDown()
	{
	}

	void Renderer::RenderToTexture(RenderTargetTexture* texture)
	{
		const OpenGLRHI& openGL = OpenGLRHI::Get();

		texture->Bind();
		openGL.Viewport(0, 0, texture->GetWidth(), texture->GetHeight());
		openGL.Clear(EGLBufferBit::Color | EGLBufferBit::Depth);
		// #TODO(Josh) How does this interact with other shaders? Will this cause problems? What is the best way to achieve this in a robust manner?
		renderToTextureShader->Use();
		for (auto& renderItem : mRenderList)
		{
			if (renderItem.bIsValid)
			{
				RenderSingleItem(renderItem);
			}
		}
		openGL.Viewport(0, 0, mCanvasSize.X(), mCanvasSize.Y());
		texture->Unbind();
	}

	void Renderer::ClearLists()
	{
	}

	void Renderer::EnableVsync(bool bEnabled)
	{
		OpenGLRHI::Get().SetSwapInterval(bEnabled ? 1 : 0);
	}

	void Renderer::ResizeCanvas(const Vector2D& newSize)
	{
		mCanvasSize = newSize;
		OpenGLRHI::Get().Viewport(0, 0, static_cast<GLsizei>(newSize.X()), static_cast<GLsizei>(newSize.Y()));
	}

	void Renderer::RenderSingleItem(RenderItemProtocol& renderItem)
	{
		const OpenGLRHI& openGL = OpenGLRHI::Get();

		// #NOTE(Josh) Need to handle this via sorting to set only once.
		renderItem.Shader->Use();
		renderItem.Shader->SetUniformMatrix4x4("UProjectionMat", camera.ProjectionMat);
		renderItem.Shader->SetUniformMatrix4x4("UViewMat", camera.ViewMat);
		renderItem.Shader->SetUniformVector4D("UFragColor", renderItem.Material.Color);
		renderItem.Shader->SetUniformMatrix4x4("UModelMat", renderItem.ModelMat);
		
		renderItem.Shader->SetUniformInt("Material.Diffuse", 0);
		renderItem.Shader->SetUniformInt("Material.Specular", 1);
		
		renderItem.Shader->SetUniformInt("UNumActiveLights", mLightingList.size());
		renderItem.Shader->SetUniformVector3D(std::string("ViewPos").c_str(), camera.Position);

		for (size_t lightIdx = 0; lightIdx < mLightingList.size(); ++lightIdx)
		{
			const LightItemProtocol& lightItem = mLightingList[lightIdx];
			std::string itemIdxStr = Conversions::StringFromInt(static_cast<int>(lightIdx));

			renderItem.Shader->SetUniformVector3D(std::string("LightPositions[" + itemIdxStr + "]").c_str(), lightItem.Position);
			renderItem.Shader->SetUniformVector3D(std::string("LightColors[" + itemIdxStr + "]").c_str(), lightItem.Color);
			renderItem.Shader->SetUniformFloat(std::string("LightStrengths[" + itemIdxStr + "]").c_str(), lightItem.Strength);
		}

		const std::vector<GFXMesh*>& meshList = *renderItem.MeshData;
		for (auto& mesh : meshList)
		{
			const std::vector<GFXTexture2D*>& diffuseTextures = mesh->GetDiffuseTextures();
			const std::vector<GFXTexture2D*>& specularTextures = mesh->GetSpecularTextures();

			renderItem.Shader->SetUniformInt("DiffuseTextureCount", static_cast<int>(diffuseTextures.size()));
			renderItem.Shader->SetUniformInt("SpecularTextureCount", static_cast<int>(specularTextures.size()));
			
			int textureCount = 0;
			for (size_t i = 0; i < diffuseTextures.size(); ++i, ++textureCount)
			{
				renderItem.Shader->SetUniformInt(("Material.DiffuseTextures[0]"), diffuseTextures[i]->GetTextureID());
				glActiveTexture(GL_TEXTURE0 + textureCount);
				openGL.BindTexture(EGLCapability::Texture2D, diffuseTextures[i]->GetTextureID());
			}

			for (size_t i = 0; i < specularTextures.size(); ++i, ++textureCount)
			{
				renderItem.Shader->SetUniformInt(("Material.SpecularTextures[0]"), specularTextures[i]->GetTextureID());
				glActiveTexture(GL_TEXTURE0 + textureCount);
				openGL.BindTexture(EGLCapability::Texture2D, specularTextures[i]->GetTextureID());
			}

			mesh->GetVAO().Bind();

			OpenGLRHI::Get().DrawElements(EGLDrawMode::Triangles, mesh->GetIndices().size(), EGLDataType::UnsignedInt, nullptr);

			mesh->GetVAO().Unbind();

			openGL.BindTexture(EGLCapability::Texture2D, 0);
		}
	}

	void Renderer::RenderToTexture_Test(RenderItemProtocol& itemProtocol)
	{
		const OpenGLRHI& openGL = OpenGLRHI::Get();

		renderToTextureShader->Use();
		renderToTextureShader->SetUniformMatrix4x4("UProjectionMat", camera.ProjectionMat);
		renderToTextureShader->SetUniformMatrix4x4("UViewMat", camera.ViewMat);
		renderToTextureShader->SetUniformMatrix4x4("UModelMat", itemProtocol.ModelMat);

		for (auto& light : mLightingList)
		{
			renderToTextureShader->SetUniformVector3D("ULightPosition", light.Position);
			renderToTextureShader->SetUniformVector3D("UViewPosition", camera.Position);
			renderToTextureShader->SetUniformVector3D("ULightColor", light.Color);
			renderToTextureShader->SetUniformFloat("ULightStrength", light.Strength);
		}

		const std::vector<GFXMesh*>& meshList = *itemProtocol.MeshData;
		for (auto& mesh : meshList)
		{
			mesh->GetVAO().Bind();

			OpenGLRHI::Get().DrawElements(EGLDrawMode::Triangles, mesh->GetIndices().size(), EGLDataType::UnsignedInt, nullptr);

			mesh->GetVAO().Unbind();
		}
		openGL.BindTexture(GL_TEXTURE_2D, 0);
	}

	Renderer::RenderItemProtocol::RenderItemProtocol()
	{
		MeshData = nullptr;
	}
}
