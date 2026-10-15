#include <StdAfx.h>
#include <Game/World/GameObject/Properties/PropertyPath.h>

#include <Utils/DebugUtils/Debug.h>

PropertyPath::PropertyPath(size_t propertyIndex)
{
	Step& step = m_steps.emplace_back();
	step.PropertyIndex = propertyIndex;
}

PropertyPath PropertyPath::Nested(const std::string& elementKey, size_t propertyIndex) const
{
	AssertMsg(!IsEmpty(), "A nested path starts from a component's property");

	PropertyPath nested = *this;
	nested.m_steps.back().ElementKey = elementKey;

	Step& step = nested.m_steps.emplace_back();
	step.PropertyIndex = propertyIndex;
	return nested;
}

bool PropertyPath::operator==(const PropertyPath& other) const
{
	if (m_steps.size() != other.m_steps.size())
	{
		return false;
	}

	for (size_t stepIndex = 0; stepIndex < m_steps.size(); ++stepIndex)
	{
		const Step& step = m_steps[stepIndex];
		const Step& otherStep = other.m_steps[stepIndex];
		// The last step's ElementKey is unused, so it doesn't count
		const bool isLast = stepIndex + 1 == m_steps.size();
		if (step.PropertyIndex != otherStep.PropertyIndex || (!isLast && step.ElementKey != otherStep.ElementKey))
		{
			return false;
		}
	}
	return true;
}
