#pragma once

#include <Graphics/RenderStageBuilder.h>

#include <vector>

// The result of compiling the stages' declarations: what each stage declared and the order they run in.
// Never changes once built, so it can be read from anywhere while the pipeline runs.
struct CompiledRenderPipeline
{
	// Indexed like the stages that were compiled
	std::vector<RenderStageDeclaration> Declarations;
	// Indices into Declarations, in execution order
	std::vector<size_t> ExecutionOrder;
	// False if the declarations had errors (logged); the order is still usable
	bool IsValid = true;
};

// Validates the declarations and works out the execution order. For each data contract its writer runs
// first, then its modifiers in the order they were added, then its readers. Stages with no dependency
// between them keep the order they were added in.
CompiledRenderPipeline CompileRenderPipeline(std::vector<RenderStageDeclaration> declarations);
