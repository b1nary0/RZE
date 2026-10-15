#pragma once

#include <string>
#include <vector>

// Where a property is, starting from a component: one of the component's own properties, or a property of an
// element of one of its editable Arrays (and so on, nested). ComponentPropertyRegistry::Resolve follows it. Refers
// to elements by key rather than position, so it still leads to the same element after the list changes, and
// leads nowhere once that element is gone.
class PropertyPath
{
public:
	PropertyPath() = default;
	// The component's own property at propertyIndex
	explicit PropertyPath(size_t propertyIndex);

	// The property at propertyIndex of the element keyed elementKey, in the Array this path leads to
	PropertyPath Nested(const std::string& elementKey, size_t propertyIndex) const;

	// Into which Array element each step goes, then which property: steps before the last are Arrays, the last is
	// the property itself
	struct Step
	{
		size_t PropertyIndex = 0;
		// The element of this step's Array to go into next; unused on the last step
		std::string ElementKey;
	};
	const std::vector<Step>& GetSteps() const { return m_steps; }

	bool IsEmpty() const { return m_steps.empty(); }
	// A property of an Array element rather than of the component itself
	bool IsNested() const { return m_steps.size() > 1; }

	bool operator==(const PropertyPath& other) const;
	bool operator!=(const PropertyPath& other) const { return !(*this == other); }

private:
	std::vector<Step> m_steps;
};
