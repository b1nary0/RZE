#include <doctest/doctest.h>

#include <Rendering/Driver/OpenGL/GLShaderTranslator.h>

#include <Utils/Platform/Filepath.h>

#include <algorithm>
#include <filesystem>
#include <string>
#include <vector>

using Rendering::GLShaderTranslator::EStage;

namespace
{
	// The shaders the engine loads (Assets/Shaders, not the folders under it), and their stages by name
	std::vector<std::pair<std::string, EStage>> GetShaders()
	{
		std::vector<std::pair<std::string, EStage>> shaders;
		for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(Filepath("Assets/Shaders").GetAbsolutePath()))
		{
			const std::string name = entry.path().filename().string();
			if (entry.path().extension() != ".hlsl")
			{
				continue;
			}
			if (name.rfind("Vertex_", 0) == 0)
			{
				shaders.emplace_back(name, EStage::Vertex);
			}
			else if (name.rfind("Pixel_", 0) == 0)
			{
				shaders.emplace_back(name, EStage::Pixel);
			}
			else if (name.rfind("Compute_", 0) == 0)
			{
				shaders.emplace_back(name, EStage::Compute);
			}
		}
		return shaders;
	}

	// The shaders are in the source tree, not beside the test executable
	struct SourceTreePaths
	{
		SourceTreePaths() { Filepath::SetDirectoryContext(EDirectoryContext::Source); }
		~SourceTreePaths() { Filepath::SetDirectoryContext(EDirectoryContext::Runtime); }
	};
}

TEST_CASE("Every HLSL shader translates to GLSL for the OpenGL renderer")
{
	const SourceTreePaths sourceTree;
	const std::vector<std::pair<std::string, EStage>> shaders = GetShaders();
	REQUIRE_FALSE(shaders.empty());

	for (const std::pair<std::string, EStage>& shader : shaders)
	{
		CAPTURE(shader.first);
		Rendering::GLShaderTranslator::Result result;
		std::string errors;
		CHECK_MESSAGE(Rendering::GLShaderTranslator::Translate(Filepath("Assets/Shaders/" + shader.first), shader.second, result, errors), errors);
		CHECK_FALSE(result.GLSL.empty());
	}
}

TEST_CASE("Every pixel shader has a vertex shader whose outputs are where it reads them")
{
	const SourceTreePaths sourceTree;
	// Each stage is its own program, so a pixel shader's input only meets the vertex shader's output of the same
	// semantic if both are at the same location. Which vertex shader each pixel shader is drawn with is up to the render
	// stages, so this only checks that there's one it could be.
	std::vector<std::pair<std::string, Rendering::GLShaderTranslator::Result>> vertexShaders;
	std::vector<std::pair<std::string, Rendering::GLShaderTranslator::Result>> pixelShaders;
	for (const std::pair<std::string, EStage>& shader : GetShaders())
	{
		if (shader.second == EStage::Compute)
		{
			continue;
		}

		Rendering::GLShaderTranslator::Result result;
		std::string errors;
		REQUIRE(Rendering::GLShaderTranslator::Translate(Filepath("Assets/Shaders/" + shader.first), shader.second, result, errors));
		(shader.second == EStage::Vertex ? vertexShaders : pixelShaders).emplace_back(shader.first, std::move(result));
	}

	for (const auto& pixelShader : pixelShaders)
	{
		const bool hasMatchingVertexShader = std::any_of(vertexShaders.begin(), vertexShaders.end(), [&pixelShader](const auto& vertexShader)
			{
				const std::vector<Rendering::GLShaderTranslator::Varying>& outputs = vertexShader.second.Outputs;
				return std::all_of(pixelShader.second.Inputs.begin(), pixelShader.second.Inputs.end(), [&outputs](const Rendering::GLShaderTranslator::Varying& input)
					{
						return std::any_of(outputs.begin(), outputs.end(), [&input](const Rendering::GLShaderTranslator::Varying& output)
							{
								return output.Semantic == input.Semantic && output.Location == input.Location;
							});
					});
			});
		CAPTURE(pixelShader.first);
		CHECK(hasMatchingVertexShader);
	}
}
