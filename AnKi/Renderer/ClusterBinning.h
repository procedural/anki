// Copyright (C) 2009-2021, Panagiotis Christopoulos Charitos and contributors.
// All rights reserved.
// Code licensed under the BSD License.
// http://www.anki3d.org/LICENSE

#pragma once

#include <AnKi/Renderer/RendererObject.h>

namespace anki
{

/// @addtogroup renderer
/// @{

/// Bins clusterer objects to the clusterer.
class ClusterBinning : public RendererObject
{
public:
	ClusterBinning(Renderer* r);

	~ClusterBinning();

	ANKI_USE_RESULT Error init(const ConfigSet& config);

	/// Populate the rendergraph.
	void populateRenderGraph(RenderingContext& ctx);

private:
	ShaderProgramResourcePtr m_prog;
	ShaderProgramPtr m_grProg;

	U32 m_clusterCount = 0;

	class
	{
	public:
		RenderingContext* m_ctx = nullptr;
	} m_runCtx;

	void run(RenderPassWorkContext& rgraphCtx);
	void writeClustererBuffers(RenderingContext& ctx);
	void writeClustererBuffersTask();
};
/// @}

} // end namespace anki
