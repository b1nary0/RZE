#include <Graphics/RenderPipeline.h>

#include <Graphics/RenderContext.h>
#include <Graphics/RenderStage.h>
#include <Graphics/RenderView.h>

#include <Utils/TimeUtils.h>

RenderPipeline::RenderPipeline() = default;
RenderPipeline::~RenderPipeline() = default;

void RenderPipeline::AddStage(std::unique_ptr<IRenderStage> stage)
{
	AssertNotNull(stage);

	stage->Initialize();
	m_stages.push_back(std::move(stage));
	m_stageTimes.push_back(0.0);
	m_isDirty = true;
}

void RenderPipeline::Clear()
{
	m_stages.clear();
	m_stageTimes.clear();
	m_compiled = CompiledRenderPipeline();
	m_isDirty = true;
}

void RenderPipeline::Render(RenderView& view, RenderScene& scene, float deltaTime, U64 frameIndex)
{
	CompileIfDirty();

	view.m_frameBlackboard.BeginFrame();
	RenderContext context(view, scene, deltaTime, frameIndex);

	for (size_t stageIndex : m_compiled.ExecutionOrder)
	{
		const bool runsInView = m_compiled.Declarations[stageIndex].ViewFilter == ERenderViewFilter::All || view.IsMainView();
		if (runsInView)
		{
			const double stageStart = TimeUtils::GetSeconds();
			m_stages[stageIndex]->Render(context);
			m_stageTimes[stageIndex] += TimeUtils::GetSeconds() - stageStart;
		}
	}
}

std::vector<RenderStageTime> RenderPipeline::TakeStageTimes()
{
	std::vector<RenderStageTime> stageTimes;
	stageTimes.reserve(m_compiled.ExecutionOrder.size());
	for (size_t stageIndex : m_compiled.ExecutionOrder)
	{
		stageTimes.push_back({ m_stages[stageIndex]->GetName(), m_stageTimes[stageIndex] * 1000.0 });
		m_stageTimes[stageIndex] = 0.0;
	}

	return stageTimes;
}

void RenderPipeline::CompileIfDirty()
{
	if (!m_isDirty)
	{
		return;
	}

	std::vector<RenderStageDeclaration> declarations(m_stages.size());
	for (size_t stageIndex = 0; stageIndex < m_stages.size(); ++stageIndex)
	{
		declarations[stageIndex].StageName = m_stages[stageIndex]->GetName();

		RenderStageBuilder builder(declarations[stageIndex]);
		m_stages[stageIndex]->Setup(builder);
	}

	m_compiled = CompileRenderPipeline(std::move(declarations));
	AssertMsg(m_compiled.IsValid, "Render stage declarations are invalid; see the log for details");

	std::string order;
	for (size_t stageIndex : m_compiled.ExecutionOrder)
	{
		order += order.empty() ? "" : " -> ";
		order += m_stages[stageIndex]->GetName();
	}
	RZE_LOG_ARGS("Render pipeline: %s", order.c_str());

	m_isDirty = false;
}
