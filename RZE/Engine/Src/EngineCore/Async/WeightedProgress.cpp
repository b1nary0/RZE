#include <StdAfx.h>
#include <EngineCore/Async/WeightedProgress.h>

#include <algorithm>

U32 WeightedProgress::AddStage(float weight)
{
	AssertExpr(m_stageCount < k_maxStages);
	AssertExpr(weight >= 0.0f);

	Stage& stage = m_stages[m_stageCount];
	stage = Stage();
	stage.Weight = weight;

	return m_stageCount++;
}

void WeightedProgress::SetStage(U32 stage, U32 completed, U32 total)
{
	AssertExpr(stage < m_stageCount);

	m_stages[stage].Completed = completed;
	m_stages[stage].Total = total;
}

void WeightedProgress::MarkStageComplete(U32 stage)
{
	AssertExpr(stage < m_stageCount);

	m_stages[stage].IsComplete = true;
}

float WeightedProgress::Compute() const
{
	float totalWeight = 0.0f;
	float weightedSum = 0.0f;

	for (U32 stageIndex = 0; stageIndex < m_stageCount; ++stageIndex)
	{
		const Stage& stage = m_stages[stageIndex];

		float fraction = 0.0f;
		if (stage.IsComplete)
		{
			fraction = 1.0f;
		}
		else if (stage.Total > 0)
		{
			fraction = std::min(1.0f, static_cast<float>(stage.Completed) / static_cast<float>(stage.Total));
		}

		totalWeight += stage.Weight;
		weightedSum += stage.Weight * fraction;
	}

	return totalWeight > 0.0f ? weightedSum / totalWeight : 0.0f;
}
