#include <Graphics/RenderView.h>

void RenderView::ReleaseStageState()
{
	m_frame.Clear();
	m_persistent.Clear();
}
