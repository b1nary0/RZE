//#include <StdAfx.h>
#include <Diotima/Renderer.h>

#include <Diotima/Driver/OpenGL.h>
#include <Diotima/Graphics/Material.h>
#include <Diotima/Graphics/Mesh.h>
#include <Diotima/Graphics/RenderTarget.h>
#include <Diotima/Graphics/Texture2D.h>
#include <Diotima/Shaders/ShaderPipeline.h>

#include <Utils/Conversions.h>
#include <Utils/DebugUtils/Debug.h>
#include <Utils/Math/Vector4D.h>
#include <Utils/Platform/FilePath.h>

#include <imGUI/imgui.h>

//
// <ImGui>
void ImGUIRender(ImDrawData* drawData);
bool ImGUICreateFontsTexture();
bool ImGUICreateDeviceObjects();
// </ImGui>
//

Diotima::GFXShaderPipeline* renderToTextureShader = nullptr;
void CreateRenderToTextureShader()
{
	const FilePath vertShaderFilePath("Engine/Assets/Shaders/RenderToTextureVert.shader");
	const FilePath fragShaderFilePath("Engine/Assets/Shaders/RenderToTextureFrag.shader");

	Diotima::GFXShader* vertShader = new Diotima::GFXShader(EGLShaderType::Vertex, "RenderToTextureVertex");
	vertShader->Load(vertShaderFilePath);
	vertShader->Create();
	vertShader->Compile();

	Diotima::GFXShader* fragShader = new Diotima::GFXShader(EGLShaderType::Fragment, "RenderToTextureFragment");
	fragShader->Load(fragShaderFilePath);
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

		// Meshing renderer with ImGui because of new application/engine relationship experimental but also because... why not, just make it with the renderer for the time being
		{
			ImGui::SetCurrentContext(ImGui::CreateContext());

			ImGuiIO& io = ImGui::GetIO();
			ImGUICreateDeviceObjects();
			io.RenderDrawListsFn = ImGUIRender;
		}
	}

	void Renderer::Update()
	{
		AssertNotNull(mRenderTarget);

		const OpenGLRHI& openGL = OpenGLRHI::Get();

		mRenderTarget->Bind();
		// #TODO(Josh) Can probably optimize this away nicely
		openGL.Viewport(0, 0, mRenderTarget->GetWidth(), mRenderTarget->GetHeight());
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
		openGL.Viewport(0, 0, static_cast<GLint>(mCanvasSize.X()), static_cast<GLint>(mCanvasSize.Y()));
		mRenderTarget->Unbind();
	}

	void Renderer::ShutDown()
	{
	}

	void Renderer::SetRenderTarget(RenderTarget* renderTarget)
	{
		AssertNotNull(renderTarget);
		mRenderTarget = renderTarget;
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
		// TODO(Josh)
		// This whole function is a temporary implementation until an actual render pipeline is implemented.
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

			// #TODO(Josh) These string constructions need to be refactored, too slow.
			renderItem.Shader->SetUniformVector3D("LightPositions[0]", lightItem.Position);
			renderItem.Shader->SetUniformVector3D("LightColors[0]", lightItem.Color);
			renderItem.Shader->SetUniformFloat("LightStrengths[0]", lightItem.Strength);
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








//
// <Imgui Stuff>
//
// #TODO(Josh) Move this to a more sane place
// This is basically just here until I figure out the best place to put this -- cause the renderer doesnt really know about editor or no editor.

static GLuint gFontTexture = 0;

static int gShaderHandle = 0;
static int gVertHandle = 0;
static int gFragHandle = 0;

static int gAttribLocationTex = 0;
static int gAttribLocationProjMtx = 0;
static int gAttribLocationPosition = 0;
static int gAttribLocationUV = 0;
static int gAttribLocationColor = 0;

static unsigned int gVBOHandle = 0;
static unsigned int gVAOHandle = 0;
static unsigned int gElementsHandle = 0;

void ImGUIRender(ImDrawData* drawData)
{
	ImGuiIO& io = ImGui::GetIO();
	int fbWidth = (int)(io.DisplaySize.x * io.DisplayFramebufferScale.x);
	int fbHeight = (int)(io.DisplaySize.y * io.DisplayFramebufferScale.y);
	if (fbWidth == 0 || fbHeight == 0)
		return;
	drawData->ScaleClipRects(io.DisplayFramebufferScale);

	// Backup GL state
	GLenum last_active_texture; glGetIntegerv(GL_ACTIVE_TEXTURE, (GLint*)&last_active_texture);
	glActiveTexture(GL_TEXTURE0);
	GLint last_program; glGetIntegerv(GL_CURRENT_PROGRAM, &last_program);
	GLint last_texture; glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
	GLint last_sampler; glGetIntegerv(GL_SAMPLER_BINDING, &last_sampler);
	GLint last_array_buffer; glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &last_array_buffer);
	GLint last_element_array_buffer; glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &last_element_array_buffer);
	GLint last_vertex_array; glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &last_vertex_array);
	GLint last_polygon_mode[2]; glGetIntegerv(GL_POLYGON_MODE, last_polygon_mode);
	GLint last_viewport[4]; glGetIntegerv(GL_VIEWPORT, last_viewport);
	GLint last_scissor_box[4]; glGetIntegerv(GL_SCISSOR_BOX, last_scissor_box);
	GLenum last_blend_src_rgb; glGetIntegerv(GL_BLEND_SRC_RGB, (GLint*)&last_blend_src_rgb);
	GLenum last_blend_dst_rgb; glGetIntegerv(GL_BLEND_DST_RGB, (GLint*)&last_blend_dst_rgb);
	GLenum last_blend_src_alpha; glGetIntegerv(GL_BLEND_SRC_ALPHA, (GLint*)&last_blend_src_alpha);
	GLenum last_blend_dst_alpha; glGetIntegerv(GL_BLEND_DST_ALPHA, (GLint*)&last_blend_dst_alpha);
	GLenum last_blend_equation_rgb; glGetIntegerv(GL_BLEND_EQUATION_RGB, (GLint*)&last_blend_equation_rgb);
	GLenum last_blend_equation_alpha; glGetIntegerv(GL_BLEND_EQUATION_ALPHA, (GLint*)&last_blend_equation_alpha);
	GLboolean last_enable_blend = glIsEnabled(GL_BLEND);
	GLboolean last_enable_cull_face = glIsEnabled(GL_CULL_FACE);
	GLboolean last_enable_depth_test = glIsEnabled(GL_DEPTH_TEST);
	GLboolean last_enable_scissor_test = glIsEnabled(GL_SCISSOR_TEST);

	// Setup render state: alpha-blending enabled, no face culling, no depth testing, scissor enabled, polygon fill
	glEnable(GL_BLEND);
	glBlendEquation(GL_FUNC_ADD);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_SCISSOR_TEST);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	const float ortho_projection[4][4] =
	{
		{ 2.0f / io.DisplaySize.x, 0.0f,                   0.0f, 0.0f },
	{ 0.0f,                  2.0f / -io.DisplaySize.y, 0.0f, 0.0f },
	{ 0.0f,                  0.0f,                  -1.0f, 0.0f },
	{ -1.0f,                  1.0f,                   0.0f, 1.0f },
	};

	glUseProgram(gShaderHandle);
	glUniform1i(gAttribLocationTex, 0);
	glUniformMatrix4fv(gAttribLocationProjMtx, 1, GL_FALSE, &ortho_projection[0][0]);
	glBindVertexArray(gVAOHandle);
	glBindSampler(0, 0); // Rely on combined texture/sampler state.

	for (int n = 0; n < drawData->CmdListsCount; n++)
	{
		const ImDrawList* cmd_list = drawData->CmdLists[n];
		const ImDrawIdx* idx_buffer_offset = 0;

		glBindBuffer(GL_ARRAY_BUFFER, gVBOHandle);
		glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)cmd_list->VtxBuffer.Size * sizeof(ImDrawVert), (const GLvoid*)cmd_list->VtxBuffer.Data, GL_STREAM_DRAW);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gElementsHandle);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx), (const GLvoid*)cmd_list->IdxBuffer.Data, GL_STREAM_DRAW);

		for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
		{
			const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
			if (pcmd->UserCallback)
			{
				pcmd->UserCallback(cmd_list, pcmd);
			}
			else
			{
				glBindTexture(GL_TEXTURE_2D, (GLuint)(intptr_t)pcmd->TextureId);
				glScissor((int)pcmd->ClipRect.x, (int)(fbHeight - pcmd->ClipRect.w), (int)(pcmd->ClipRect.z - pcmd->ClipRect.x), (int)(pcmd->ClipRect.w - pcmd->ClipRect.y));
				glDrawElements(GL_TRIANGLES, (GLsizei)pcmd->ElemCount, sizeof(ImDrawIdx) == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT, idx_buffer_offset);
			}
			idx_buffer_offset += pcmd->ElemCount;
		}
	}

	// Restore modified GL state
	glUseProgram(last_program);
	glBindTexture(GL_TEXTURE_2D, last_texture);
	glBindSampler(0, last_sampler);
	glActiveTexture(last_active_texture);
	glBindVertexArray(last_vertex_array);
	glBindBuffer(GL_ARRAY_BUFFER, last_array_buffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, last_element_array_buffer);
	glBlendEquationSeparate(last_blend_equation_rgb, last_blend_equation_alpha);
	glBlendFuncSeparate(last_blend_src_rgb, last_blend_dst_rgb, last_blend_src_alpha, last_blend_dst_alpha);
	if (last_enable_blend) glEnable(GL_BLEND); else glDisable(GL_BLEND);
	if (last_enable_cull_face) glEnable(GL_CULL_FACE); else glDisable(GL_CULL_FACE);
	if (last_enable_depth_test) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
	if (last_enable_scissor_test) glEnable(GL_SCISSOR_TEST); else glDisable(GL_SCISSOR_TEST);
	glPolygonMode(GL_FRONT_AND_BACK, last_polygon_mode[0]);
	glViewport(last_viewport[0], last_viewport[1], (GLsizei)last_viewport[2], (GLsizei)last_viewport[3]);
	glScissor(last_scissor_box[0], last_scissor_box[1], (GLsizei)last_scissor_box[2], (GLsizei)last_scissor_box[3]);
}

bool ImGUICreateFontsTexture()
{
	// Build texture atlas
	ImGuiIO& io = ImGui::GetIO();
	unsigned char* pixels;
	int width, height;
	io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);   // Load as RGBA 32-bits (75% of the memory is wasted, but default font is so small) because it is more likely to be compatible with user's existing shaders. If your ImTextureId represent a higher-level concept than just a GL texture id, consider calling GetTexDataAsAlpha8() instead to save on GPU memory.

															  // Upload texture to graphics system
	GLint last_texture;
	glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
	glGenTextures(1, &gFontTexture);
	glBindTexture(GL_TEXTURE_2D, gFontTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

	// Store our identifier
	io.Fonts->TexID = (void *)(intptr_t)gFontTexture;

	// Restore state
	glBindTexture(GL_TEXTURE_2D, last_texture);

	return true;
}

bool ImGUICreateDeviceObjects()
{
	// Backup GL state
	GLint last_texture, last_array_buffer, last_vertex_array;
	glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
	glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &last_array_buffer);
	glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &last_vertex_array);

	const GLchar *vertex_shader =
		"#version 330\n"
		"uniform mat4 ProjMtx;\n"
		"in vec2 Position;\n"
		"in vec2 UV;\n"
		"in vec4 Color;\n"
		"out vec2 Frag_UV;\n"
		"out vec4 Frag_Color;\n"
		"void main()\n"
		"{\n"
		"	Frag_UV = UV;\n"
		"	Frag_Color = Color;\n"
		"	gl_Position = ProjMtx * vec4(Position.xy,0,1);\n"
		"}\n";

	const GLchar* fragment_shader =
		"#version 330\n"
		"uniform sampler2D Texture;\n"
		"in vec2 Frag_UV;\n"
		"in vec4 Frag_Color;\n"
		"out vec4 Out_Color;\n"
		"void main()\n"
		"{\n"
		"	Out_Color = Frag_Color * texture( Texture, Frag_UV.st);\n"
		"}\n";

	gShaderHandle = glCreateProgram();
	gVertHandle = glCreateShader(GL_VERTEX_SHADER);
	gFragHandle = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(gVertHandle, 1, &vertex_shader, 0);
	glShaderSource(gFragHandle, 1, &fragment_shader, 0);
	glCompileShader(gVertHandle);
	glCompileShader(gFragHandle);
	glAttachShader(gShaderHandle, gVertHandle);
	glAttachShader(gShaderHandle, gFragHandle);
	glLinkProgram(gShaderHandle);

	gAttribLocationTex = glGetUniformLocation(gShaderHandle, "Texture");
	gAttribLocationProjMtx = glGetUniformLocation(gShaderHandle, "ProjMtx");
	gAttribLocationPosition = glGetAttribLocation(gShaderHandle, "Position");
	gAttribLocationUV = glGetAttribLocation(gShaderHandle, "UV");
	gAttribLocationColor = glGetAttribLocation(gShaderHandle, "Color");

	glGenBuffers(1, &gVBOHandle);
	glGenBuffers(1, &gElementsHandle);

	glGenVertexArrays(1, &gVAOHandle);
	glBindVertexArray(gVAOHandle);
	glBindBuffer(GL_ARRAY_BUFFER, gVBOHandle);
	glEnableVertexAttribArray(gAttribLocationPosition);
	glEnableVertexAttribArray(gAttribLocationUV);
	glEnableVertexAttribArray(gAttribLocationColor);

#define OFFSETOF(TYPE, ELEMENT) ((size_t)&(((TYPE *)0)->ELEMENT))
	glVertexAttribPointer(gAttribLocationPosition, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (GLvoid*)OFFSETOF(ImDrawVert, pos));
	glVertexAttribPointer(gAttribLocationUV, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (GLvoid*)OFFSETOF(ImDrawVert, uv));
	glVertexAttribPointer(gAttribLocationColor, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(ImDrawVert), (GLvoid*)OFFSETOF(ImDrawVert, col));
#undef OFFSETOF

	ImGUICreateFontsTexture();

	// Restore modified GL state
	glBindTexture(GL_TEXTURE_2D, last_texture);
	glBindBuffer(GL_ARRAY_BUFFER, last_array_buffer);
	glBindVertexArray(last_vertex_array);

	return true;
}
//
// </Imgui Stuff>
