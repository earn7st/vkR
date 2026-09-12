#include "SkyPass.h"
#include "runtime/asset/Material.h"
#include "runtime/asset/Shader.h"
#include "runtime/render/RenderConfig.h"
#include "runtime/render/resources/RenderResourceManager.h"
#include "runtime/rhi/RHICommandList.h"

namespace shzk
{
	// SkyPass
	void SkyPass::Init()
	{
		m_meshPassProcessor = std::make_shared<SkyPassProcessor>(this);

		m_vertexShader = std::make_shared<Shader>(SHZK_SPIRV_DIR "sky.vert.spv", SHADER_FREQUENCY_VERTEX, "main");
		m_fragmentShader = std::make_shared<Shader>(SHZK_SPIRV_DIR "sky.frag.spv", SHADER_FREQUENCY_FRAGMENT, "main");

		{
			RHIRootSignatureInfo info{};
			auto perFrameRS = RenderResourceManager::Get()->GetPerFrameRootSignature();
			auto matRS = RenderResourceManager::Get()->GetMaterialRootSignature();
			assert(perFrameRS);
			assert(matRS);
			info.AddPushConstant({ .offset = 0, .size = 128, .frequency = SHADER_FREQUENCY_VERTEX });	// 这里是由冗余的，因为 sky pass 中完全用不到 per primitive 的 push constant 信息
																										// 但由于 SubmitDrawBegin 中默认 PushConstants，为了通用性就保留了；反正最终要改 bindless 舍弃 pushconstants anyway
			info.AddEntry(perFrameRS->GetInfo())
				.AddEntry(matRS->GetInfo());
			m_rootSignature = RHI::Get()->CreateRootSignature(info);
		}

		{
			m_colorAttachmentFormats.fill(FORMAT_UKNOWN);
			m_colorAttachmentFormats[0] = HDR_COLOR_FORMAT;
			m_depthStencilAttachmentFormat = DEPTH_FORMAT;
		}
	}

	void SkyPass::Prepare()
	{
		m_renderPassInfo = {};
		m_renderPassInfo.renderArea = RenderResourceManager::Get()->GetRenderExtent();
		m_renderPassInfo.layerCount = 1;
		m_renderPassInfo.viewMask = m_viewMask;

		std::shared_ptr<RHITextureView> hdrColorView =
			RenderResourceManager::Get()->GetCurrentHDRColorTextureView();
		auto& color = m_renderPassInfo.colorAttachments[0];
		color.view = hdrColorView;
		color.layout = RHIResourceState::ColorAttachment;
		color.loadOp = AttachmentLoadOp::Load;
		color.storeOp = AttachmentStoreOp::Store;
		color.clearColor = CLEAR_COLOR;

		std::shared_ptr<RHITextureView> depthView =
			RenderResourceManager::Get()->GetCurrentSceneDepthTextureView();
		auto& depth = m_renderPassInfo.depthStencilAttachment;
		depth.view = depthView;
		depth.layout = RHIResourceState::DepthStencilAttachment;
		depth.loadOp = AttachmentLoadOp::Load;
		depth.storeOp = AttachmentStoreOp::DontCare;
		depth.clearDepth = 0.f;	// reverse-z
	}

	void SkyPass::Execute(std::shared_ptr<RHICommandList> cmd)
	{
		cmd->TextureBarrier({
		  RenderResourceManager::Get()->GetCurrentHDRColorTexture(),
		  RHIResourceState::ColorAttachment,
		  RHIResourceState::ColorAttachment
			});
		cmd->TextureBarrier({
			RenderResourceManager::Get()->GetCurrentSceneDepthTexture(),
			RHIResourceState::DepthStencilAttachment,
			RHIResourceState::DepthStencilAttachment
			});
		cmd->BeginRendering(m_renderPassInfo);
		cmd->SetViewport({ 0,0 }, { m_renderPassInfo.renderArea.width, m_renderPassInfo.renderArea.height });
		cmd->SetScissor({ 0,0 }, { m_renderPassInfo.renderArea.width, m_renderPassInfo.renderArea.height });
		m_meshPassProcessor->ExecuteDrawCommands(cmd);
		cmd->EndRendering();
	}

// SkyPassProcessor
	SkyPassProcessor::SkyPassProcessor(SkyPass* pass)
		: MeshPassProcessor(), m_pass(pass)
	{
		for (auto& rt : m_renderState.blendState.renderTargets)
		{
			rt.bEnable = false;
			rt.colorWriteMask = COLOR_WRITE_MASK_RGBA;
			rt.colorBlendOp = BlendOp::Add;
			rt.alphaBlendOp = BlendOp::Add;
			rt.colorSrcBlend = BlendFactor::One;
			rt.colorDstBlend = BlendFactor::Zero;
			rt.alphaSrcBlend = BlendFactor::One;
			rt.alphaDstBlend = BlendFactor::Zero;
		}

		m_renderState.depthStencilState.bEnableDepthTest = true;
		m_renderState.depthStencilState.bEnableDepthWrite = false;
		m_renderState.depthStencilState.depthTest = CompareFunction::GreaterEqual;
	}

	void SkyPassProcessor::AddMeshBatch(const MeshBatch& batch)
	{
		std::shared_ptr<Material> material = batch.material;
		if (!material) return;
		if (material->GetPassMask() & PASS_MASK_SKY_PASS)
		{
			std::shared_ptr<Shader> vertexShader = material->GetVertexShader();
			std::shared_ptr<Shader> fragmentShader = material->GetFragmentShader();

			MeshPassProcessorRenderState renderState = m_renderState;	// baseline
			// renderState.depthStencilState.bEnableDepthTest	= material->DepthTest();
			// renderState.depthStencilState.bEnableDepthWrite = material->DepthWrite();
			// renderState.depthStencilState.depthTest			= material->GetDepthCompare();

			BuildMeshDrawCommands(
				batch,
				material,
				renderState,
				vertexShader != nullptr ? vertexShader->GetRHIShader() : m_pass->GetVertexShader()->GetRHIShader(),
				fragmentShader != nullptr ? fragmentShader->GetRHIShader() : m_pass->GetFragmentShader()->GetRHIShader(),
				material->GetRasterizerCullMode(),
				material->GetRasterizerFillMode());
		}
	}

	RHIGraphicsPipelineInfo SkyPassProcessor::BuildRHIGraphicsPipelineInfo(const GraphicsMinimalPipelineState& minimal)
	{
		RHIGraphicsPipelineInfo info{};
		info.vertexShader = minimal.boundShaderStateInput.vertexShader;
		info.fragmentShader = minimal.boundShaderStateInput.fragmentShader;
		info.rootSignature = m_pass->GetRHIRootSignature();
		info.vertexInputState = minimal.boundShaderStateInput.declaration;
		info.primitiveType = minimal.primitiveType;
		info.rasterizerState = minimal.rasterizerState;
		info.blendState = minimal.blendState;
		info.depthStencilState = minimal.depthStencilState;
		info.colorAttachmentFormats = m_pass->GetColorAttachmentFormats();
		info.depthStencilAttachmentFormat = m_pass->GetDepthStencilAttachmentFormat();
		info.viewMask = m_pass->GetViewMask();

		return info;
	}
}

