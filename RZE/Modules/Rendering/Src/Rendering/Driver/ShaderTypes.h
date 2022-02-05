#pragma once

#include <Utils/Platform/FilePath.h>

#include <Utils/DebugUtils/Debug.h>

#include <string>
#include <vector>

namespace Rendering
{
	class IGFXDevice;

	class BufferLayout
	{
	public:
		enum class EDataFormat : uint8_t
		{
			R32G32_FLOAT = 0,
			R32G32B32_FLOAT
		};

		enum class EDataClassification : uint8_t
		{
			PER_INSTANCE = 0,
			PER_VERTEX
		};

	private:
		struct BufferLayoutData
		{
			BufferLayoutData(const std::string& name, EDataFormat dataFormat, EDataClassification dataClassification, size_t size, size_t offset)
				: Name(name)
				, DataFormat(dataFormat)
				, DataClassification(dataClassification)
				, Size(size)
				, Offset(offset) {}

			std::string Name;
			EDataFormat DataFormat;
			EDataClassification DataClassification;
			size_t Size;
			size_t Offset;
		};

	public:
		BufferLayout(const std::initializer_list<BufferLayoutData>& initializerList)
			: m_layoutData(initializerList) {}

	private:
		std::vector<BufferLayoutData> m_layoutData;
	};
	
	// @TODO move into it's own file?
	class IShader
	{
	public:
		IShader() = default;
		virtual ~IShader() = default;

		virtual void Create(const FilePath& filePath) = 0;
		virtual void Release() = 0;

		virtual void SetActive() = 0;
	};
}