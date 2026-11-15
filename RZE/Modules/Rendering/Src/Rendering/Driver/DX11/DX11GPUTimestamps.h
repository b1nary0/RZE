#pragma once

#include <Utils/DebugUtils/DebugConfig.h>

// Render stats only
#ifdef RZE_RENDER_STATS

#include <Rendering/Driver/GPUTimestamps.h>

struct ID3D11Query;

namespace Rendering
{
	class DX11Device;

	// D3D11_QUERY_TIMESTAMP queries, one per timestamp, inside a D3D11_QUERY_TIMESTAMP_DISJOINT query for the frame,
	// which gives their clock's frequency and says whether it changed (the times can't be trusted then)
	class DX11GPUTimestamps : public IGPUTimestamps
	{
	public:
		DX11GPUTimestamps(DX11Device& device, U32 capacity);
		~DX11GPUTimestamps() override;

	// IGPUTimestamps interface
	public:
		void Begin() override;
		void Write(U32 index) override;
		void End() override;
		EGPUQueryReadResult Read(U32 count, std::vector<double>& outSeconds) override;

	private:
		DX11Device& m_device;
		ID3D11Query* m_disjoint = nullptr;
		std::vector<ID3D11Query*> m_timestamps;
	};
}

#endif // RZE_RENDER_STATS
