#include "IBLPass.h"
#include "runtime/asset/Shader.h"
#include "runtime/asset/Texture.h"
#include "runtime/rhi/RHI.h"
#include "runtime/rhi/RHICommandList.h"
#include "runtime/rhi/RHIResource.h"
#include "runtime/render/RenderConfig.h"
#include "runtime/render/resources/RenderResourceManager.h"

namespace shzk
{
	void IBLPass::Init()
	{
		// Resources
		m_resources[0].shader = std::make_shared<Shader>(SHZK_SPIRV_DIR "ibl_diffuse.comp.spv", SHADER_FREQUENCY_COMPUTE, "main");
		m_resources[1].shader = std::make_shared<Shader>(SHZK_SPIRV_DIR "ibl_specular.comp.spv", SHADER_FREQUENCY_COMPUTE, "main");

		RHIRootSignatureInfo info{};
		info.AddPushConstant({ .offset = 0, .size = 128, .frequency = SHADER_FREQUENCY_COMPUTE });
		info.AddEntry({ 0, 0, 1, SHADER_FREQUENCY_COMPUTE, RESOURCE_TYPE_COMBINED_IMAGE_SAMPLER })
			.AddEntry({ 0, 1, 1, SHADER_FREQUENCY_COMPUTE, RESOURCE_TYPE_RW_TEXTURE });
		m_resources[0].rs = RHI::Get()->CreateRootSignature(info);
		m_resources[1].rs = m_resources[0].rs;

		m_diffuseDescSet = m_resources[0].rs->CreateDescriptorSet(0);
		for (uint32_t mip = 0; mip < IBL_SPEC_MIPS; ++mip)
			m_specularDescSets[mip] = m_resources[1].rs->CreateDescriptorSet(0);
		
		RHIComputePipelineInfo infoDiffuse{};
		infoDiffuse.computeShader = m_resources[0].shader->GetRHIShader();
		infoDiffuse.rootSignature = m_resources[0].rs;
		m_resources[0].pipeline = RHI::Get()->CreateComputePipeline(infoDiffuse);

		RHIComputePipelineInfo infoSpecular{};
		infoSpecular.computeShader = m_resources[1].shader->GetRHIShader();
		infoSpecular.rootSignature = m_resources[1].rs;
		m_resources[1].pipeline = RHI::Get()->CreateComputePipeline(infoSpecular);

		{
			m_diffuseView = RenderResourceManager::Get()->GetIBLDiffuseTextureView();
			RHIDescriptorUpdateInfo info{};
			info.binding = 1;
			info.index = 0;
			info.resourceType = RESOURCE_TYPE_RW_TEXTURE;
			info.textureView = m_diffuseView;
			m_diffuseDescSet->UpdateDescriptor(info);
		}
		
		{
			std::shared_ptr<RHITexture> specularTex = RenderResourceManager::Get()->GetIBLSpecularTexture();
			for (uint32_t mip = 0; mip < IBL_SPEC_MIPS; ++mip) 
			{
				RHITextureViewInfo v{};
				v.texture = specularTex;
				v.format = FORMAT_R16G16B16A16_SFLOAT;
				v.viewType = TextureViewType::ViewCube;
				v.subresourceRange = { TEXTURE_ASPECT_COLOR, mip, 1, 0, 6 };
				m_specularViews[mip] = RHI::Get()->CreateTextureView(v);

				RHIDescriptorUpdateInfo info{};
				info.binding = 1;
				info.index = 0;
				info.resourceType = RESOURCE_TYPE_RW_TEXTURE;
				info.textureView = m_specularViews[mip];
				m_specularDescSets[mip]->UpdateDescriptor(info);
			}
		}

		m_brdfLUT = std::make_shared<Texture>(SHZK_ASSETS_DIR "_builtin/BRDF_LUT.png", TextureType::Type2D, FORMAT_R8G8B8A8_UNORM);
	}

	void IBLPass::Prepare()
	{}

	void IBLPass::Execute(std::shared_ptr<RHICommandList> cmd)
	{
		if (!m_envMap || !m_bEnvMapChanged) return;
		m_bEnvMapChanged = false;

		RHIDescriptorUpdateInfo envInfo{};
		envInfo.binding = 0;
		envInfo.index = 0;
		envInfo.resourceType = RESOURCE_TYPE_COMBINED_IMAGE_SAMPLER;
		envInfo.sampler = RenderResourceManager::Get()->GetDefaultSampler()->GetRHISampler();
		envInfo.textureView = m_envMap->GetRHITextureView();
		m_diffuseDescSet->UpdateDescriptor(envInfo);
		for (auto& descSet : m_specularDescSets)
		{
			descSet->UpdateDescriptor(envInfo);
		}

		cmd->TextureBarrier({ RenderResourceManager::Get()->GetIBLDiffuseTexture(),  RHIResourceState::Undefined, RHIResourceState::UnorderedAccess });
		cmd->TextureBarrier({ RenderResourceManager::Get()->GetIBLSpecularTexture(), RHIResourceState::Undefined, RHIResourceState::UnorderedAccess });

		cmd->SetComputePipeline(m_resources[0].pipeline);
		cmd->BindDescriptorSet(m_diffuseDescSet, 0);
		{
			cmd->PushConstants(&m_setting, sizeof(m_setting), SHADER_FREQUENCY_COMPUTE);
		}
		cmd->Dispatch(IBL_IRR_SIZE / 16, IBL_IRR_SIZE / 16, 6);

		cmd->SetComputePipeline(m_resources[1].pipeline);
		for (uint32_t mip = 0; mip < IBL_SPEC_MIPS; ++mip) 
		{	
			cmd->BindDescriptorSet(m_specularDescSets[mip], 0);

			uint32_t mipSize = (uint32_t)IBL_SPEC_SIZE >> mip;
			m_setting.roughness = (float)mip / (float)(IBL_SPEC_MIPS - 1);
			m_setting.mipSize = mipSize;

			cmd->PushConstants(&m_setting, sizeof(m_setting), SHADER_FREQUENCY_COMPUTE);
			cmd->Dispatch((mipSize + 15) / 16, (mipSize + 15) / 16, 6);
		}

		cmd->TextureBarrier({ RenderResourceManager::Get()->GetIBLDiffuseTexture(),  RHIResourceState::UnorderedAccess, RHIResourceState::ShaderResource });
		cmd->TextureBarrier({ RenderResourceManager::Get()->GetIBLSpecularTexture(), RHIResourceState::UnorderedAccess, RHIResourceState::ShaderResource });
	}
}