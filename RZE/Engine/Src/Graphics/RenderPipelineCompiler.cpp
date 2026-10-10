#include <StdAfx.h>
#include <Graphics/RenderPipelineCompiler.h>

#include <unordered_map>

namespace
{
	// Every stage that touches one data contract, by index into the declarations
	struct ContractUsage
	{
		const char* Name = nullptr;
		std::vector<size_t> Writers;
		std::vector<size_t> Modifiers;			// In the order they were added
		std::vector<size_t> Readers;			// Required and optional
		std::vector<size_t> RequiredReaders;
	};

	using ContractUsageMap = std::unordered_map<RenderDataTypeId, ContractUsage>;

	ContractUsageMap GatherContractUsage(const std::vector<RenderStageDeclaration>& declarations)
	{
		ContractUsageMap usages;
		for (size_t stage = 0; stage < declarations.size(); ++stage)
		{
			for (const RenderStageDeclaration::DataAccess& access : declarations[stage].Accesses)
			{
				ContractUsage& usage = usages[access.TypeId];
				usage.Name = access.Name;

				switch (access.Access)
				{
				case ERenderDataAccess::Write:			usage.Writers.push_back(stage); break;
				case ERenderDataAccess::Modify:			usage.Modifiers.push_back(stage); break;
				case ERenderDataAccess::Read:			usage.Readers.push_back(stage); usage.RequiredReaders.push_back(stage); break;
				case ERenderDataAccess::ReadOptional:	usage.Readers.push_back(stage); break;
				}
			}
		}
		return usages;
	}

	// Logs each problem and returns false if there were any
	bool Validate(const std::vector<RenderStageDeclaration>& declarations, const ContractUsageMap& usages)
	{
		bool isValid = true;

		for (const auto& [typeId, usage] : usages)
		{
			if (usage.Writers.size() > 1)
			{
				isValid = false;
				for (size_t writer : usage.Writers)
				{
					RZE_LOG_ARGS("Render pipeline: %s is written by more than one stage, including %s", usage.Name, declarations[writer].StageName);
				}
			}

			// Stages that can't run without the data
			std::vector<size_t> dependents = usage.RequiredReaders;
			dependents.insert(dependents.end(), usage.Modifiers.begin(), usage.Modifiers.end());

			if (usage.Writers.empty())
			{
				for (size_t dependent : dependents)
				{
					isValid = false;
					RZE_LOG_ARGS("Render pipeline: %s needs %s, but no stage writes it", declarations[dependent].StageName, usage.Name);
				}
				continue;
			}

			// A stage that runs in every view needs the data in secondary views too
			const RenderStageDeclaration& writer = declarations[usage.Writers.front()];
			if (writer.ViewFilter == ERenderViewFilter::MainOnly)
			{
				for (size_t dependent : dependents)
				{
					if (declarations[dependent].ViewFilter == ERenderViewFilter::All)
					{
						isValid = false;
						RZE_LOG_ARGS("Render pipeline: %s needs %s in every view, but its writer %s only runs in the main view",
							declarations[dependent].StageName, usage.Name, writer.StageName);
					}
				}
			}
		}

		return isValid;
	}

	// Kahn's algorithm. Returns false if some stages depend on each other in a cycle; they're appended in
	// the order they were added so rendering carries on.
	bool SortStages(const std::vector<RenderStageDeclaration>& declarations, const ContractUsageMap& usages, std::vector<size_t>& outOrder)
	{
		const size_t stageCount = declarations.size();

		// runsBefore[a] lists the stages that must wait for a
		std::vector<std::vector<size_t>> runsBefore(stageCount);
		std::vector<size_t> waitingOn(stageCount, 0);

		auto addEdge = [&](size_t before, size_t after)
		{
			runsBefore[before].push_back(after);
			++waitingOn[after];
		};

		for (const auto& [typeId, usage] : usages)
		{
			// Writer -> modifiers (chained in the order they were added) -> readers
			std::vector<size_t> chain = usage.Writers;
			chain.insert(chain.end(), usage.Modifiers.begin(), usage.Modifiers.end());

			for (size_t link = 1; link < chain.size(); ++link)
			{
				addEdge(chain[link - 1], chain[link]);
			}

			if (!chain.empty())
			{
				for (size_t reader : usage.Readers)
				{
					addEdge(chain.back(), reader);
				}
			}
		}

		outOrder.clear();
		std::vector<bool> isScheduled(stageCount, false);

		while (outOrder.size() < stageCount)
		{
			// The earliest-added stage that's ready, so independent stages keep their order
			size_t next = stageCount;
			for (size_t stage = 0; stage < stageCount; ++stage)
			{
				if (!isScheduled[stage] && waitingOn[stage] == 0)
				{
					next = stage;
					break;
				}
			}

			if (next == stageCount)
			{
				for (size_t stage = 0; stage < stageCount; ++stage)
				{
					if (!isScheduled[stage])
					{
						RZE_LOG_ARGS("Render pipeline: %s is part of a dependency cycle", declarations[stage].StageName);
						outOrder.push_back(stage);
						isScheduled[stage] = true;
					}
				}
				return false;
			}

			outOrder.push_back(next);
			isScheduled[next] = true;
			for (size_t dependent : runsBefore[next])
			{
				--waitingOn[dependent];
			}
		}

		return true;
	}
}

CompiledRenderPipeline CompileRenderPipeline(std::vector<RenderStageDeclaration> declarations)
{
	CompiledRenderPipeline compiled;

	const ContractUsageMap usages = GatherContractUsage(declarations);
	const bool isValid = Validate(declarations, usages);
	const bool isAcyclic = SortStages(declarations, usages, compiled.ExecutionOrder);

	compiled.Declarations = std::move(declarations);
	compiled.IsValid = isValid && isAcyclic;
	return compiled;
}
