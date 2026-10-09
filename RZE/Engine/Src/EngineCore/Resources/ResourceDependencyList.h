#pragma once

#include <functional>
#include <string>
#include <vector>

#include <EngineCore/Resources/AsyncResourceBatch.h>

// A type-erased list of resources to preload. Built on any thread (e.g. while parsing a scene on a worker)
// and replayed into an AsyncResourceBatch on the main thread.
class ResourceDependencyList
{
public:
	template <class TResource>
	void Add(const std::string& resourcePath)
	{
		m_requests.emplace_back([resourcePath](AsyncResourceBatch& batch)
			{
				batch.Request<TResource>(Filepath(resourcePath));
			});
	}

	// Main thread only.
	void RequestAll(AsyncResourceBatch& batch) const
	{
		for (const std::function<void(AsyncResourceBatch&)>& request : m_requests)
		{
			request(batch);
		}
	}

	size_t GetCount() const { return m_requests.size(); }

private:
	std::vector<std::function<void(AsyncResourceBatch&)>> m_requests;
};
