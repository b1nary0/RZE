#include <Rendering/Driver/DX11/DX11GPUStatistics.h>

// Render stats only (its header is empty without them)
#ifdef RZE_RENDER_STATS

#include <Rendering/Driver/DX11/DX11Device.h>

#include <Rendering/Driver/DX11/DX11.h>

#include <Utils/DebugUtils/Debug.h>

namespace Rendering
{
	DX11GPUStatistics::DX11GPUStatistics(DX11Device& device, U32 capacity)
		: m_device(device)
		, m_queries(capacity, nullptr)
	{
		ID3D11Device& hardwareDevice = m_device.GetHardwareDevice();

		D3D11_QUERY_DESC desc = {};
		desc.Query = D3D11_QUERY_PIPELINE_STATISTICS;
		for (ID3D11Query*& query : m_queries)
		{
			const HRESULT hr = hardwareDevice.CreateQuery(&desc, &query);
			AssertExpr(hr == S_OK);
		}
	}

	DX11GPUStatistics::~DX11GPUStatistics()
	{
		for (ID3D11Query* query : m_queries)
		{
			if (query != nullptr)
			{
				query->Release();
			}
		}
	}

	void DX11GPUStatistics::Begin()
	{
		AssertExpr(m_openQuery == nullptr);
	}

	void DX11GPUStatistics::BeginSegment(U32 index)
	{
		AssertExpr(m_openQuery == nullptr);
		AssertExpr(index < m_queries.size());

		m_openQuery = m_queries[index];
		m_device.GetDeviceContext().Begin(m_openQuery);
	}

	void DX11GPUStatistics::EndSegment()
	{
		AssertNotNull(m_openQuery);

		m_device.GetDeviceContext().End(m_openQuery);
		m_openQuery = nullptr;
	}

	void DX11GPUStatistics::End()
	{
		AssertExpr(m_openQuery == nullptr);
	}

	EGPUQueryReadResult DX11GPUStatistics::Read(U32 count, std::vector<GPUPipelineStatistics>& outSegments)
	{
		AssertExpr(count <= m_queries.size());
		ID3D11DeviceContext& context = m_device.GetDeviceContext();

		outSegments.resize(count);
		for (U32 segment = 0; segment < count; ++segment)
		{
			// S_FALSE until the GPU has finished the segment. DONOTFLUSH: asking mustn't make the driver submit work early.
			D3D11_QUERY_DATA_PIPELINE_STATISTICS data = {};
			const HRESULT hr = context.GetData(m_queries[segment], &data, sizeof(data), D3D11_ASYNC_GETDATA_DONOTFLUSH);
			if (hr == S_FALSE)
			{
				return EGPUQueryReadResult::NotReady;
			}
			if (hr != S_OK)
			{
				return EGPUQueryReadResult::Invalid;
			}

			GPUPipelineStatistics& statistics = outSegments[segment];
			statistics.TrianglesSubmitted = static_cast<double>(data.IAPrimitives);
			statistics.VerticesShaded = static_cast<double>(data.VSInvocations);
			statistics.TrianglesInView = static_cast<double>(data.CPrimitives);
			statistics.PixelsShaded = static_cast<double>(data.PSInvocations);
		}
		return EGPUQueryReadResult::Ready;
	}
}

#endif // RZE_RENDER_STATS
