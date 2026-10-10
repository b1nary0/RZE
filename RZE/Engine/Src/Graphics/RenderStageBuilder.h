#pragma once

#include <Graphics/RenderData/RenderStageData.h>
#include <Graphics/RenderPorts.h>

#include <Utils/DebugUtils/Debug.h>

#include <vector>

enum class ERenderDataAccess
{
	Read,			// Must be written by another stage; runs after its writer and modifiers
	ReadOptional,	// As Read, but the stage copes without it (TryGet)
	Write,			// The single producer; runs before every reader and modifier
	Modify			// Reads and writes back an existing value (e.g. drawing into a target); runs after the writer, before readers
};

enum class ERenderViewFilter
{
	All,
	MainOnly		// Skipped for secondary views such as camera previews
};

// What a stage declared in IRenderStage::Setup(). The pipeline compiler uses it to validate and order the stages.
struct RenderStageDeclaration
{
	struct DataAccess
	{
		RenderDataTypeId TypeId;
		const char* Name;
		ERenderDataAccess Access;
	};

	const char* StageName = nullptr;
	std::vector<DataAccess> Accesses;
	ERenderViewFilter ViewFilter = ERenderViewFilter::All;
};

// Passed to IRenderStage::Setup() for a stage to declare the data contracts it uses. Each call returns the
// port the stage then reads or writes that contract through (RenderPorts.h).
class RenderStageBuilder
{
public:
	explicit RenderStageBuilder(RenderStageDeclaration& declaration)
		: m_declaration(declaration) {}

	template <typename T> RenderInput<T> Reads() { Declare<T>(ERenderDataAccess::Read); return MakePort<RenderInput<T>>(); }
	template <typename T> RenderInput<T> ReadsOptional() { Declare<T>(ERenderDataAccess::ReadOptional); return MakePort<RenderInput<T>>(); }
	template <typename T> RenderOutput<T> Writes() { Declare<T>(ERenderDataAccess::Write); return MakePort<RenderOutput<T>>(); }
	template <typename T> RenderInOut<T> Modifies() { Declare<T>(ERenderDataAccess::Modify); return MakePort<RenderInOut<T>>(); }

	void RunsIn(ERenderViewFilter filter) { m_declaration.ViewFilter = filter; }

private:
	template <typename T>
	void Declare(ERenderDataAccess access)
	{
		static_assert(IsRenderStageData<T>, "Render stage data must derive from RenderStageData<T>");

		for (const RenderStageDeclaration::DataAccess& existing : m_declaration.Accesses)
		{
			AssertMsg(existing.TypeId != T::TypeId(), "A stage may declare each data contract only once");
		}

		m_declaration.Accesses.push_back({ T::TypeId(), T::Name(), access });
	}

	template <typename TPort>
	static TPort MakePort()
	{
		TPort port;
		port.m_isDeclared = true;
		return port;
	}

private:
	RenderStageDeclaration& m_declaration;
};
