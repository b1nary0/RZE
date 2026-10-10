#pragma once

#include <Graphics/RenderContext.h>
#include <Graphics/RenderPorts.h>
#include <Graphics/RenderStageBuilder.h>

// A pass in the render pipeline.
//
// Writing a render stage
// ----------------------
// Stages never reference each other. They exchange data contracts: small structs of handles and values,
// each in its own header in Graphics/RenderData/ (see RenderStageData.h).
//
//	1. In Setup(), declare every contract the stage uses and keep the ports as members:
//		- Reads<T>() / ReadsOptional<T>(): data another stage writes
//		- Writes<T>(): new data. Each contract has exactly one writer; new data means a new contract.
//		- Modifies<T>(): existing data the stage changes in place, such as drawing into a target
//	   and call RunsIn(ERenderViewFilter::MainOnly) if the stage must skip secondary views.
//	   The pipeline orders the stages from these declarations and reports any that don't fit together.
//	2. In Render(), use the ports for that data and RenderContext for everything else (view inputs,
//	   scene, frame timing). Add new per-frame inputs to RenderContext rather than reading globals.
//	3. Stage members are shared by every view. Keep only things that are the same for every view
//	   (shaders, layouts, scratch resources used within one Render() call). State that must carry over
//	   between frames for a particular view belongs in context.Persistent().
//
// Views currently render one after another on the main thread, so scratch GPU resources (e.g. the shadow
// map) can be stage members.
class IRenderStage
{
public:
	IRenderStage() {};
	virtual ~IRenderStage() {};

public:
	// Used in validation errors and the logged stage order
	virtual const char* GetName() const = 0;

	virtual void Initialize() = 0;
	// Declares the data this stage uses and which views it runs in. Called each time the pipeline is compiled.
	virtual void Setup(RenderStageBuilder& builder) = 0;

	virtual void Render(RenderContext& context) = 0;
};
