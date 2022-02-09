#pragma once

#include <Rendering/Driver/TypeDefines.h>

#include <Utils/Platform/FilePath.h>
#include <Utils/DebugUtils/Debug.h>

#include <string>
#include <vector>

namespace Rendering
{
	class IGFXDevice;

	class ShaderInputLayout
	{
	public:
		enum class EDataClassification : uint8_t
		{
			PER_INSTANCE = 0,
			PER_VERTEX
		};

		struct ShaderInputLayoutData
		{
			ShaderInputLayoutData(const std::string& name, EDataFormat dataFormat, EDataClassification dataClassification, size_t offset)
				: Name(name)
				, DataFormat(dataFormat)
				, DataClassification(dataClassification)
				, Offset(offset) {}

			std::string Name;
			EDataFormat DataFormat;
			EDataClassification DataClassification;
			size_t Offset;
		};

	public:
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

		virtual void Create(const FilePath& filePath, const ShaderInputLayout& inputLayout) = 0;
		virtual void Release() = 0;
		virtual void SetActive() = 0;
		virtual void SetInputLayout() = 0;
	};

	// @TODO move into it's own file?
	class IPixelShader
	{
	public:
		IPixelShader() = default;
		virtual ~IPixelShader() = default;

		virtual void Create(const FilePath& filePath) = 0;
		virtual void Release() = 0;

		virtual void SetActive() = 0;
	};
}