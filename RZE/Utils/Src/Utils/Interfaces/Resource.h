#pragma once

#include <Utils/Platform/Filepath.h>

// Main-thread cost of a single IResource::FinalizeStep(), used to time-slice GPU submission across frames.
struct ResourceFinalizeCost
{
	// Bytes the step will copy into the per-frame render command arena (hard limit).
	size_t ArenaBytes = 0;
	// Bytes the render thread will upload to the GPU as a result of the step (soft limit).
	size_t UploadBytes = 0;
};

class AsyncResourceBatch;

class IResource
{
public:
	IResource() = default;
	virtual ~IResource() = default;

	virtual bool Load(const Filepath& filePath) = 0;
	virtual void Release() = 0;

	// @TODO This is gross, bespoke texture params on IResource. Figure it out.
	virtual bool Load(const U8* buffer, int width, int height) { return false; }

	//
	// Optional two-phase loading, used by AsyncResourceBatch to load resources without stalling the main thread.
	// Resources that opt in should implement Load() as LoadCPU() followed by FinalizeStep() until it returns true.
	//
public:
	virtual bool SupportsAsyncLoad() const { return false; }

	// Any thread. CPU-only work (file I/O, parsing, decoding). Must not touch the renderer,
	// the ResourceHandler, ResourceHandles or any other engine state.
	virtual bool LoadCPU(const Filepath& filePath) { return false; }

	// Main thread, after LoadCPU succeeded. Request any resources that must be loaded before this one
	// can be finalized (e.g. a mesh's textures).
	virtual void RequestDependencies(AsyncResourceBatch& batch) const {}

	// Main thread. Cost of the next FinalizeStep(), so it can be deferred to a later frame if it won't fit.
	virtual ResourceFinalizeCost GetNextFinalizeStepCost() const { return ResourceFinalizeCost(); }

	// Main thread. Performs one bounded unit of GPU submission. Returns true once finalization is complete.
	virtual bool FinalizeStep() { return true; }
};
