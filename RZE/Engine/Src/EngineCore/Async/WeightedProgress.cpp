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

	m_stages[stage].Fraction = total > 0 ? std::min(1.0f, static_cast<float>(completed) / static_cast<float>(total)) : 0.0f;
}

void WeightedProgress::SetStageFraction(U32 stage, float fraction)
{
	AssertExpr(stage < m_stageCount);

	m_stages[stage].Fraction = std::clamp(fraction, 0.0f, 1.0f);
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

		const float fraction = stage.IsComplete ? 1.0f : stage.Fraction;

		totalWeight += stage.Weight;
		weightedSum += stage.Weight * fraction;
	}

	return totalWeight > 0.0f ? weightedSum / totalWeight : 0.0f;
}
