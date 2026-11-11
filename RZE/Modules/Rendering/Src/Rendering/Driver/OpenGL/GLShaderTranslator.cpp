#include <Rendering/Driver/OpenGL/GLShaderTranslator.h>

#include <Utils/Platform/Filepath.h>

#include <OpenGL/SPIRV-Cross/spirv_glsl.hpp>

#include <shaderc/shaderc.h>

#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>

namespace Rendering
{
	namespace
	{
		// Where each stage's b registers start among the uniform block bindings (GLShaderTranslator.h). D3D has 14 b
		// registers per stage; GL has at least 72 bindings.
		constexpr U32 k_pixelUniformBlockBase = 16;
		constexpr U32 k_computeUniformBlockBase = 32;
		// Where the vertex shader's combined samplers start among the texture units. GL has at least 32 per draw, and D3D
		// 16 texture registers per stage.
		constexpr U32 k_vertexTextureUnitBase = 16;

		std::string ReadFile(const std::filesystem::path& path)
		{
			std::ifstream file(path, std::ios::binary);
			return std::string(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
		}

		// An include shaderc asked for, kept until it lets go of it
		struct Include
		{
			std::string Name;
			std::string Content;
			shaderc_include_result Result = {};
		};

		// Beside the shader being translated (Assets/Shaders), never beside the file including it: D3D's standard include
		// handler doesn't look there, so an include only GL could find would fail on Windows
		shaderc_include_result* ResolveInclude(void* shaderFolder, const char* requested, int /*type*/, const char* /*requesting*/, size_t /*depth*/)
		{
			std::unique_ptr<Include> include = std::make_unique<Include>();
			const std::filesystem::path path = *static_cast<const std::filesystem::path*>(shaderFolder) / requested;

			if (std::filesystem::exists(path))
			{
				include->Name = path.string();
				include->Content = ReadFile(path);
			}
			else
			{
				// An empty name tells shaderc it wasn't found, and the content says why
				include->Content = std::string("Can't find the include ") + requested;
			}

			include->Result.source_name = include->Name.c_str();
			include->Result.source_name_length = include->Name.size();
			include->Result.content = include->Content.c_str();
			include->Result.content_length = include->Content.size();
			include->Result.user_data = include.get();
			return &include.release()->Result;
		}

		void ReleaseInclude(void* /*shaderFolder*/, shaderc_include_result* result)
		{
			delete static_cast<Include*>(result->user_data);
		}

		// SPIRV-Cross's GLSL, with what HLSL allows and GLSL doesn't written the GLSL way
		class HLSLCompilerGLSL : public spirv_cross::CompilerGLSL
		{
		public:
			using spirv_cross::CompilerGLSL::CompilerGLSL;

			// Every RWTexture the shaders write is write-only, which GLSL lets go without a format; each store then
			// converts to the bound texture's own format, as D3D's do. glslang gives each a 32-bit float format from its
			// HLSL type instead (e.g. rgba32f for float4), which needn't be the texture's. A shader that reads one fails
			// to compile, since GLSL needs the format for that.
			void ClearImageFormats()
			{
				for (const spirv_cross::Resource& image : get_shader_resources().storage_images)
				{
					get<spirv_cross::SPIRType>(image.base_type_id).image.format = spv::ImageFormatUnknown;
					get<spirv_cross::SPIRType>(image.type_id).image.format = spv::ImageFormatUnknown;
				}
			}

		protected:
			// HLSL's SampleCmpLevelZero on a TextureCubeArray: GLSL has no textureLod or textureGrad for a
			// samplerCubeArrayShadow, which SPIRV-Cross would use. Plain texture() reads mip 0 outside pixel shaders, and
			// shadow cubes only have mip 0, so the constant 0 LOD is left out for them.
			bool IsCubeArrayCompareAtLodZero(const spirv_cross::SPIRType& imageType, bool hasCompare, uint32_t lod, bool isFetch)
			{
				if (!(imageType.image.dim == spv::DimCube && imageType.image.arrayed && hasCompare && lod != 0 && !isFetch))
				{
					return false;
				}
				if (!expression_is_constant_null(lod))
				{
					throw spirv_cross::CompilerError("A compare on a TextureCubeArray at a mip other than 0 can't be written in GLSL");
				}
				return true;
			}

			std::string to_function_name(const TextureFunctionNameArguments& args) override
			{
				if (IsCubeArrayCompareAtLodZero(*args.base.imgtype, args.has_dref, args.lod, args.base.is_fetch))
				{
					TextureFunctionNameArguments atLodZero = args;
					atLodZero.lod = 0;
					return CompilerGLSL::to_function_name(atLodZero);
				}
				return CompilerGLSL::to_function_name(args);
			}

			std::string to_function_args(const TextureFunctionArguments& args, bool* p_forward) override
			{
				if (IsCubeArrayCompareAtLodZero(*args.base.imgtype, args.dref != 0, args.lod, args.base.is_fetch))
				{
					TextureFunctionArguments atLodZero = args;
					atLodZero.lod = 0;
					return CompilerGLSL::to_function_args(atLodZero, p_forward);
				}
				return CompilerGLSL::to_function_args(args, p_forward);
			}
		};

		bool CompileToSPIRV(const std::filesystem::path& path, GLShaderTranslator::EStage stage, std::vector<uint32_t>& outSPIRV, std::string& outErrors)
		{
			shaderc_shader_kind kind = shaderc_vertex_shader;
			const char* entryPoint = "VSMain";
			switch (stage)
			{
			case GLShaderTranslator::EStage::Pixel: kind = shaderc_fragment_shader; entryPoint = "PSMain"; break;
			case GLShaderTranslator::EStage::Compute: kind = shaderc_compute_shader; entryPoint = "CSMain"; break;
			case GLShaderTranslator::EStage::Vertex: break;
			}

			const std::string source = ReadFile(path);
			if (source.empty())
			{
				outErrors = "Can't read the file";
				return false;
			}

			std::filesystem::path shaderFolder = path.parent_path();
			shaderc_compiler_t compiler = shaderc_compiler_initialize();
			shaderc_compile_options_t options = shaderc_compile_options_initialize();
			shaderc_compile_options_set_source_language(options, shaderc_source_language_hlsl);
			// Vulkan's rules, which allow the HLSL's separate textures and samplers
			shaderc_compile_options_set_target_env(options, shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_0);
			// Constant buffers packed as HLSL packs them, which the C++ structs that fill them follow
			shaderc_compile_options_set_hlsl_offsets(options, true);
			shaderc_compile_options_set_hlsl_io_mapping(options, true);
			// Keeps each input's and output's HLSL semantic, which the input layouts name
			shaderc_compile_options_set_hlsl_functionality1(options, true);
			// Unoptimized, so the GLSL keeps the HLSL's names for reading in a graphics debugger; the driver optimizes it
			shaderc_compile_options_set_optimization_level(options, shaderc_optimization_level_zero);
			shaderc_compile_options_add_macro_definition(options, "RZE_SPIRV", 9, "1", 1);
			shaderc_compile_options_set_include_callbacks(options, &ResolveInclude, &ReleaseInclude, &shaderFolder);

			const std::string pathText = path.string();
			shaderc_compilation_result_t result = shaderc_compile_into_spv(compiler, source.c_str(), source.size(), kind, pathText.c_str(), entryPoint, options);
			const bool isCompiled = shaderc_result_get_compilation_status(result) == shaderc_compilation_status_success;
			if (isCompiled)
			{
				outSPIRV.resize(shaderc_result_get_length(result) / sizeof(uint32_t));
				std::memcpy(outSPIRV.data(), shaderc_result_get_bytes(result), outSPIRV.size() * sizeof(uint32_t));
			}
			else
			{
				outErrors = shaderc_result_get_error_message(result);
			}

			shaderc_result_release(result);
			shaderc_compile_options_release(options);
			shaderc_compiler_release(compiler);
			return isCompiled;
		}
	}

	namespace GLShaderTranslator
	{
		U32 GetUniformBlockBase(EStage stage)
		{
			switch (stage)
			{
			case EStage::Pixel: return k_pixelUniformBlockBase;
			case EStage::Compute: return k_computeUniformBlockBase;
			case EStage::Vertex:
			default: return 0;
			}
		}

		U32 GetTextureUnitBase(EStage stage)
		{
			return stage == EStage::Vertex ? k_vertexTextureUnitBase : 0;
		}

		bool Translate(const Filepath& file, EStage stage, Result& outResult, std::string& outErrors)
		{
			std::vector<uint32_t> spirv;
			if (!CompileToSPIRV(std::filesystem::path(file.GetAbsolutePath()), stage, spirv, outErrors))
			{
				return false;
			}

			try
			{
				HLSLCompilerGLSL compiler(std::move(spirv));
				spirv_cross::CompilerGLSL::Options options = compiler.get_common_options();
				options.version = 450;
				options.es = false;
				options.vulkan_semantics = false;
				// Each stage is its own program, bound into a program pipeline, as D3D binds stages separately
				options.separate_shader_objects = true;
				compiler.set_common_options(options);

				compiler.ClearImageFormats();
				const spirv_cross::VariableID dummySampler = compiler.build_dummy_sampler_for_combined_images();
				compiler.build_combined_image_samplers();

				U32 unit = GetTextureUnitBase(stage);
				for (const spirv_cross::CombinedImageSampler& combined : compiler.get_combined_image_samplers())
				{
					compiler.set_decoration(combined.combined_id, spv::DecorationBinding, unit);

					CombinedSampler binding;
					binding.Unit = unit++;
					binding.TextureSlot = compiler.get_decoration(combined.image_id, spv::DecorationBinding);
					binding.SamplerSlot = combined.sampler_id == dummySampler ? -1 : static_cast<int>(compiler.get_decoration(combined.sampler_id, spv::DecorationBinding));
					outResult.CombinedSamplers.push_back(binding);
				}

				const spirv_cross::ShaderResources resources = compiler.get_shader_resources();
				const U32 uniformBlockBase = GetUniformBlockBase(stage);
				for (const spirv_cross::Resource& uniformBuffer : resources.uniform_buffers)
				{
					const U32 registerIndex = compiler.get_decoration(uniformBuffer.id, spv::DecorationBinding);
					compiler.set_decoration(uniformBuffer.id, spv::DecorationBinding, uniformBlockBase + registerIndex);
				}

				for (const spirv_cross::Resource& input : resources.stage_inputs)
				{
					outResult.Inputs.push_back({ compiler.get_decoration_string(input.id, spv::DecorationUserSemantic), compiler.get_decoration(input.id, spv::DecorationLocation) });
				}
				for (const spirv_cross::Resource& output : resources.stage_outputs)
				{
					outResult.Outputs.push_back({ compiler.get_decoration_string(output.id, spv::DecorationUserSemantic), compiler.get_decoration(output.id, spv::DecorationLocation) });
				}

				for (const spirv_cross::BuiltInResource& builtin : resources.builtin_outputs)
				{
					if (builtin.builtin == spv::BuiltInClipDistance)
					{
						// An array of floats, one a distance (a float4 SV_ClipDistance0 is 4)
						const spirv_cross::SPIRType& type = compiler.get_type(builtin.value_type_id);
						outResult.ClipDistanceCount = type.array.empty() ? 1u : static_cast<U32>(type.array[0]);
					}
				}

				outResult.GLSL = compiler.compile();
			}
			catch (const spirv_cross::CompilerError& error)
			{
				outErrors = error.what();
				return false;
			}
			return true;
		}
	}
}
