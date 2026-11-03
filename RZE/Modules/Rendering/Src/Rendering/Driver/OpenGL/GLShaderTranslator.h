#pragma once

#include <Utils/PrimitiveDefs.h>

#include <string>
#include <vector>

class Filepath;

namespace Rendering
{
	// The HLSL shaders as GLSL 4.50, so OpenGL runs the same source as Direct3D: shaderc compiles the HLSL to SPIR-V,
	// and SPIRV-Cross writes that out as GLSL. Run when a shader is created, as the DX11 backend compiles then too.
	//
	// D3D binds each stage's resources by register (b, t, s, u); GL binds programs' by unit. What GLDevice needs to
	// bind a D3D register's resource where the GLSL reads it comes back with the GLSL:
	//  - a constant buffer at bN is uniform block binding N + GetUniformBlockBase(stage);
	//  - D3D's separate textures and samplers become GL's combined ones, one unit per pair the shader uses (CombinedSampler);
	//  - a compute shader's RWTexture at uN is image unit N.
	namespace GLShaderTranslator
	{
		enum class EStage
		{
			Vertex,
			Pixel,
			Compute
		};

		// A texture register read through a sampler register, at a GL texture unit
		struct CombinedSampler
		{
			U32 Unit = 0;
			U32 TextureSlot = 0;
			// -1 for a texture only read with Load (texelFetch), which needs no sampler
			int SamplerSlot = -1;
		};

		// A vertex or pixel shader's input or output, by its HLSL semantic (e.g. "UV", "POSITION1")
		struct Varying
		{
			std::string Semantic;
			U32 Location = 0;
		};

		struct Result
		{
			std::string GLSL;
			std::vector<CombinedSampler> CombinedSamplers;
			// The vertex shader's inputs (its input layout's semantics) and outputs, and the pixel shader's inputs
			std::vector<Varying> Inputs;
			std::vector<Varying> Outputs;
		};

		// Where a stage's constant buffer registers start among GL's uniform block bindings. D3D's stages each have
		// their own b registers, GL's programs share one set of bindings, so the pixel shader's are moved past the
		// vertex shader's.
		U32 GetUniformBlockBase(EStage stage);

		// The shader at file, whose entry point is VSMain, PSMain or CSMain as on D3D. Includes are found beside the file
		// including them, then beside file. False, with outErrors saying why, if it can't be made GLSL.
		bool Translate(const Filepath& file, EStage stage, Result& outResult, std::string& outErrors);
	}
}
