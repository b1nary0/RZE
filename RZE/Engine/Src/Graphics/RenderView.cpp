#include <Graphics/RenderView.h>

void RenderView::ReleaseStageState()
{
	m_frameBlackboard.Clear();
	m_persistentBlackboard.Clear();
}
