#include <Rendering/Driver/DX11/DX11GPUTimestamps.h>

// Render stats only (its header is empty without them)
#ifdef RZE_RENDER_STATS

#include <Rendering/Driver/DX11/DX11Device.h>

#include <Rendering/Driver/DX11/DX11.h>

#include <Utils/DebugUtils/Debug.h>

namespace Rendering
{
	DX11GPUTimestamps::DX11GPUTimestamps(DX11Device& device, U32 capacity)
		: m_device(device)
		, m_timestamps(capacity, nullptr)
	{
		ID3D11Device& hardwareDevice = m_device.GetHardwareDevice();

		D3D11_QUERY_DESC desc = {};
		desc.Query = D3D11_QUERY_TIMESTAMP_DISJOINT;
		HRESULT hr = hardwareDevice.CreateQuery(&desc, &m_disjoint);
		AssertExpr(hr == S_OK);

		desc.Query = D3D11_QUERY_TIMESTAMP;
		for (ID3D11Query*& timestamp : m_timestamps)
		{
			hr = hardwareDevice.CreateQuery(&desc, &timestamp);
			AssertExpr(hr == S_OK);
		}
	}

	DX11GPUTimestamps::~DX11GPUTimestamps()
	{
		for (ID3D11Query* timestamp : m_timestamps)
		{
			if (timestamp != nullptr)
			{
				timestamp->Release();
			}
		}

		if (m_disjoint != nullptr)
		{
			m_disjoint->Release();
		}
	}

	void DX11GPUTimestamps::Begin()
	{
		m_device.GetDeviceContext().Begin(m_disjoint);
	}

	void DX11GPUTimestamps::Write(U32 index)
	{
		AssertExpr(index < m_timestamps.size());
		// A timestamp query has no Begin: End is where the GPU writes it
		m_device.GetDeviceContext().End(m_timestamps[index]);
	}

	void DX11GPUTimestamps::End()
	{
		m_device.GetDeviceContext().End(m_disjoint);
	}

	EGPUQueryReadResult DX11GPUTimestamps::Read(U32 count, std::vector<double>& outSeconds)
	{
		AssertExpr(count > 0 && count <= m_timestamps.size());
		ID3D11DeviceContext& context = m_device.GetDeviceContext();

		// S_FALSE until the GPU has finished the frame. DONOTFLUSH: asking mustn't make the driver submit work early.
		D3D11_QUERY_DATA_TIMESTAMP_DISJOINT disjoint = {};
		if (context.GetData(m_disjoint, &disjoint, sizeof(disjoint), D3D11_ASYNC_GETDATA_DONOTFLUSH) != S_OK)
		{
			return EGPUQueryReadResult::NotReady;
		}
		if (disjoint.Disjoint || disjoint.Frequency == 0)
		{
			return EGPUQueryReadResult::Invalid;
		}

		outSeconds.resize(count);
		UINT64 first = 0;
		for (U32 index = 0; index < count; ++index)
		{
			UINT64 ticks = 0;
			// Done with the frame, so done with its timestamps too; a missing one means it can't be read
			if (context.GetData(m_timestamps[index], &ticks, sizeof(ticks), D3D11_ASYNC_GETDATA_DONOTFLUSH) != S_OK)
			{
				return EGPUQueryReadResult::Invalid;
			}
			if (index == 0)
			{
				first = ticks;
			}
			outSeconds[index] = static_cast<double>(ticks - first) / static_cast<double>(disjoint.Frequency);
		}
		return EGPUQueryReadResult::Ready;
	}
}

#endif // RZE_RENDER_STATS
