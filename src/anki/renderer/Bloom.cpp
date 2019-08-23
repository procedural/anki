// Copyright (C) 2009-2019, Panagiotis Christopoulos Charitos and contributors.
// All rights reserved.
// Code licensed under the BSD License.
// http://www.anki3d.org/LICENSE

#include <anki/renderer/Bloom.h>
#include <anki/renderer/DownscaleBlur.h>
#include <anki/renderer/FinalComposite.h>
#include <anki/renderer/Renderer.h>
#include <anki/renderer/Tonemapping.h>
#include <anki/misc/ConfigSet.h>

namespace anki
{

Bloom::Bloom(Renderer* r)
	: RendererObject(r)
{
}

Bloom::~Bloom()
{
}

Error Bloom::initExposure(const ConfigSet& config)
{
	m_exposure.m_width = m_r->getDownscaleBlur().getPassWidth(MAX_U32) * 2;
	m_exposure.m_height = m_r->getDownscaleBlur().getPassHeight(MAX_U32) * 2;

	m_exposure.m_threshold = config.getNumberF32("r.bloom.threshold");
	m_exposure.m_scale = config.getNumberF32("r.bloom.scale");

	// Create RT info
	m_exposure.m_rtDescr =
		m_r->create2DRenderTargetDescription(m_exposure.m_width, m_exposure.m_height, RT_PIXEL_FORMAT, "Bloom Exp");
	m_exposure.m_rtDescr.bake();

	// init shaders
	ANKI_CHECK(getResourceManager().loadResource("shaders/Bloom.glslp", m_exposure.m_prog));

	ShaderProgramResourceConstantValueInitList<2> consts(m_exposure.m_prog);
	consts.add("FB_SIZE", UVec2(m_exposure.m_width, m_exposure.m_height))
		.add("WORKGROUP_SIZE", UVec2(m_workgroupSize[0], m_workgroupSize[1]));

	const ShaderProgramResourceVariant* variant;
	m_exposure.m_prog->getOrCreateVariant(consts.get(), variant);
	m_exposure.m_grProg = variant->getProgram();

	return Error::NONE;
}

Error Bloom::initUpscale(const ConfigSet& config)
{
	m_upscale.m_width = m_r->getWidth() / BLOOM_FRACTION;
	m_upscale.m_height = m_r->getHeight() / BLOOM_FRACTION;

	// Create RT descr
	m_upscale.m_rtDescr =
		m_r->create2DRenderTargetDescription(m_upscale.m_width, m_upscale.m_height, RT_PIXEL_FORMAT, "Bloom Upscale");
	m_upscale.m_rtDescr.bake();

	// init shaders
	ANKI_CHECK(getResourceManager().loadResource("shaders/BloomUpscale.glslp", m_upscale.m_prog));

	ShaderProgramResourceConstantValueInitList<3> consts(m_upscale.m_prog);
	consts.add("FB_SIZE", UVec2(m_upscale.m_width, m_upscale.m_height))
		.add("WORKGROUP_SIZE", UVec2(m_workgroupSize[0], m_workgroupSize[1]))
		.add("INPUT_TEX_SIZE", UVec2(m_exposure.m_width, m_exposure.m_height));

	const ShaderProgramResourceVariant* variant;
	m_upscale.m_prog->getOrCreateVariant(consts.get(), variant);
	m_upscale.m_grProg = variant->getProgram();

	// Textures
	ANKI_CHECK(getResourceManager().loadResource("engine_data/LensDirt.ankitex", m_upscale.m_lensDirtTex));

	return Error::NONE;
}

void Bloom::populateRenderGraph(RenderingContext& ctx)
{
	RenderGraphDescription& rgraph = ctx.m_renderGraphDescr;

	// Main pass
	{
		// Ask for render target
		m_runCtx.m_exposureRt = rgraph.newRenderTarget(m_exposure.m_rtDescr);

		// Set the render pass
		ComputeRenderPassDescription& rpass = rgraph.newComputeRenderPass("Bloom Main");
		rpass.setWork(
			[](RenderPassWorkContext& rgraphCtx) { static_cast<Bloom*>(rgraphCtx.m_userData)->runExposure(rgraphCtx); },
			this,
			0);

		TextureSubresourceInfo inputTexSubresource;
		inputTexSubresource.m_firstMipmap = m_r->getDownscaleBlur().getMipmapCount() - 1;
		rpass.newDependency({m_r->getDownscaleBlur().getRt(), TextureUsageBit::SAMPLED_COMPUTE, inputTexSubresource});
		rpass.newDependency({m_runCtx.m_exposureRt, TextureUsageBit::IMAGE_COMPUTE_WRITE});
	}

	// Upscale & SSLF pass
	{
		// Ask for render target
		m_runCtx.m_upscaleRt = rgraph.newRenderTarget(m_upscale.m_rtDescr);

		// Set the render pass
		ComputeRenderPassDescription& rpass = rgraph.newComputeRenderPass("Bloom Upscale");
		rpass.setWork(
			[](RenderPassWorkContext& rgraphCtx) {
				static_cast<Bloom*>(rgraphCtx.m_userData)->runUpscaleAndSslf(rgraphCtx);
			},
			this,
			0);

		rpass.newDependency({m_runCtx.m_exposureRt, TextureUsageBit::SAMPLED_COMPUTE});
		rpass.newDependency({m_runCtx.m_upscaleRt, TextureUsageBit::IMAGE_COMPUTE_WRITE});
	}
}

void Bloom::runExposure(RenderPassWorkContext& rgraphCtx)
{
	CommandBufferPtr& cmdb = rgraphCtx.m_commandBuffer;

	cmdb->bindShaderProgram(m_exposure.m_grProg);

	TextureSubresourceInfo inputTexSubresource;
	inputTexSubresource.m_firstMipmap = m_r->getDownscaleBlur().getMipmapCount() - 1;

	cmdb->bindSampler(0, 0, m_r->getSamplers().m_trilinearClamp);
	rgraphCtx.bindTexture(0, 1, m_r->getDownscaleBlur().getRt(), inputTexSubresource);

	Vec4 uniforms(m_exposure.m_threshold, m_exposure.m_scale, 0.0f, 0.0f);
	cmdb->setPushConstants(&uniforms, sizeof(uniforms));

	rgraphCtx.bindStorageBuffer(0, 2, m_r->getTonemapping().getAverageLuminanceBuffer());

	rgraphCtx.bindImage(0, 3, m_runCtx.m_exposureRt, TextureSubresourceInfo());

	dispatchPPCompute(cmdb, m_workgroupSize[0], m_workgroupSize[1], m_exposure.m_width, m_exposure.m_height);
}

void Bloom::runUpscaleAndSslf(RenderPassWorkContext& rgraphCtx)
{
	CommandBufferPtr& cmdb = rgraphCtx.m_commandBuffer;

	cmdb->bindShaderProgram(m_upscale.m_grProg);

	cmdb->bindSampler(0, 0, m_r->getSamplers().m_trilinearClamp);
	rgraphCtx.bindColorTexture(0, 1, m_runCtx.m_exposureRt);
	cmdb->bindTexture(0, 2, m_upscale.m_lensDirtTex->getGrTextureView(), TextureUsageBit::SAMPLED_COMPUTE);

	rgraphCtx.bindImage(0, 3, m_runCtx.m_upscaleRt, TextureSubresourceInfo());

	dispatchPPCompute(cmdb, m_workgroupSize[0], m_workgroupSize[1], m_upscale.m_width, m_upscale.m_height);
}

} // end namespace anki
