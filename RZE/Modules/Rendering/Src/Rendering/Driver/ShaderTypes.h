#pragma once

#include <Rendering/Driver/TypeDefines.h>

#include <Utils/Platform/Filepath.h>
#include <Utils/DebugUtils/Debug.h>

#include <string>
#include <vector>

namespace Rendering
{
	class IGFXDevice;

	class ShaderInputLayout
	{
	public:
		struct ShaderInputLayoutData
		{
			// inputSlot: the vertex buffer slot it's read from (Renderer::SetVertexBuffer), e.g. 1 for a second stream
			// of per-instance data beside the vertices
			ShaderInputLayoutData(const std::string& name, EDataFormat dataFormat, EDataClassification dataClassification, size_t offset, U32 inputSlot = 0)
				: Name(name)
				, DataFormat(dataFormat)
				, DataClassification(dataClassification)
				, Offset(offset)
				, InputSlot(inputSlot) {}

			std::string Name;
			EDataFormat DataFormat;
			// PER_INSTANCE data steps on once per instance
			EDataClassification DataClassification;
			size_t Offset;
			U32 InputSlot;
		};

	public:
		ShaderInputLayout() = default;

		ShaderInputLayout(const std::initializer_list<ShaderInputLayoutData>& initializerList)
			: m_layoutData(initializerList) {}

		const ShaderInputLayoutData& operator[](size_t idx) const
		{
			return m_layoutData[idx];
		}

	public:
		size_t GetDataCount() const { return m_layoutData.size(); }

	private:
		std::vector<ShaderInputLayoutData> m_layoutData;
	};

	class IVertexShader
	{
	public:
		IVertexShader() = default;
		virtual ~IVertexShader() = default;

		virtual void Create(const Filepath& filePath, const ShaderInputLayout& inputLayout) = 0;
		virtual void Release() = 0;
		virtual void SetActive() = 0;
		virtual void SetInputLayout() = 0;
		// False until created, and when it failed to compile
		virtual bool IsCreated() const = 0;
	};

	// @TODO move into it's own file?
	class IPixelShader
	{
	public:
		IPixelShader() = default;
		virtual ~IPixelShader() = default;

		virtual void Create(const Filepath& filePath) = 0;
		virtual void Release() = 0;

		virtual void SetActive() = 0;
	};

	class IComputeShader
	{
	public:
		IComputeShader() = default;
		virtual ~IComputeShader() = default;

		virtual void Create(const Filepath& filePath) = 0;
		virtual void Release() = 0;

		virtual void SetActive() = 0;
		// False until created, and when it failed to compile
		virtual bool IsCreated() const = 0;
	};
}