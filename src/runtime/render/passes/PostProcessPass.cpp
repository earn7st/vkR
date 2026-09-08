#include "PostProcessPass.h"

#include "runtime/asset/Shader.h"
#include "runtime/rhi/RHI.h"
#include "runtime/rhi/RHICommandList.h"
#include "runtime/rhi/RHIResource.h"
#include "runtime/render/resources/RenderResourceManager.h"

namespace shzk
{
	void PostProcessPass::Init()
	{
		m_shader = std::make_shared<Shader>(SHZK_SPIRV_DIR "post_process.comp.spv", SHADER_FREQUENCY_COMPUTE, "main");
		{
			RHIRootSignatureInfo info{};
			info.AddPushConstant({ .offset = 0, .size = 128, .frequency = SHADER_FREQUENCY_COMPUTE });
			info.AddEntry({ 0, 0, 1, SHADER_FREQUENCY_COMPUTE, RESOURCE_TYPE_RW_TEXTURE })
				.AddEntry({ 0, 1, 1, SHADER_FREQUENCY_COMPUTE, RESOURCE_TYPE_RW_TEXTURE });
				// .AddEntry({ 0, 2, 1, SHADER_FREQUENCY_COMPUTE, RESOURCE_TYPE_BUFFER });
			m_rootSignature = RHI::Get()->CreateRootSignature(info);

			for (auto& descriptorSet : m_descriptorSets)
			{
				descriptorSet = m_rootSignature->CreateDescriptorSet(0);
			}
		}

		for (int i = 0; i < FRAMES_IN_FLIGHT; ++i)
		{
			{
				RHIDescriptorUpdateInfo info{};
				info.binding = 0;
				info.index = 0;
				info.resourceType = RESOURCE_TYPE_RW_TEXTURE;
				info.textureView = RenderResourceManager::Get()->GetHDRColorTextureView(i);
				m_descriptorSets[i]->UpdateDescriptor(info);
			}
			{
				RHIDescriptorUpdateInfo info{};
				info.binding = 1;
				info.index = 0;
				info.resourceType = RESOURCE_TYPE_RW_TEXTURE;
				info.textureView = RenderResourceManager::Get()->GetSceneColorTextureView(i);
				m_descriptorSets[i]->UpdateDescriptor(info);
			}
		}

		{
			RHIComputePipelineInfo info{};
			info.computeShader = m_shader->GetRHIShader();
			info.rootSignature = m_rootSignature;
			m_pipeline = RHI::Get()->CreateComputePipeline(info);
		}
	}

	void PostProcessPass::Prepare()
	{}

	void PostProcessPass::Execute(std::shared_ptr<RHICommandList> cmd)
	{
		cmd->TextureBarrier({
			RenderResourceManager::Get()->GetCurrentHDRColorTexture(),
			RHIResourceState::ColorAttachment,
			RHIResourceState::Common
			});

		cmd->TextureBarrier({
			RenderResourceManager::Get()->GetCurrentSceneColorTexture(),
			RHIResourceState::Undefined,
			RHIResourceState::Common
			});
		cmd->SetComputePipeline(m_pipeline);
		cmd->BindDescriptorSet(m_descriptorSets[RenderResourceManager::Get()->GetCurrentFrameIndex()], 0);

		Extent2D extent = RenderResourceManager::Get()->GetRenderExtent();
		uint32_t groupX = (extent.width + 16 - 1) / 16;
		uint32_t groupY = (extent.height + 16 - 1) / 16;
		cmd->Dispatch(groupX, groupY, 1);
	}
}