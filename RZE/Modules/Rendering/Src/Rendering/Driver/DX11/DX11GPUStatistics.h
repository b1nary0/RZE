#pragma once

#include <Rendering/Driver/GPUStatistics.h>

struct ID3D11Query;

namespace Rendering
{
	class DX11Device;

	// D3D11_QUERY_PIPELINE_STATISTICS queries, one a segment, which count every statistic at once
	class DX11GPUStatistics : public IGPUStatistics
	{
	public:
		DX11GPUStatistics(DX11Device& device, U32 capacity);
		~DX11GPUStatistics() override;

	// IGPUStatistics interface
	public:
		void Begin() override;
		void BeginSegment(U32 index) override;
		void EndSegment() override;
		void End() override;
		EGPUQueryReadResult Read(U32 count, std::vector<GPUPipelineStatistics>& outSegments) override;

	private:
		DX11Device& m_device;
		std::vector<ID3D11Query*> m_queries;
		// The segment begun and not yet ended, if any
		ID3D11Query* m_openQuery = nullptr;
	};
}
