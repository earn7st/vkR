#include "IBLPass.h"
#include "runtime/asset/Shader.h"
#include "runtime/asset/Texture.h"
#include "runtime/rhi/RHI.h"
#include "runtime/rhi/RHICommandList.h"
#include "runtime/rhi/RHIResource.h"
#include "runtime/render/resources/RenderResourceManager.h"

namespace shzk
{
	void IBLPass::Init()
	{
		// Resources
		m_resources[0].shader = std::make_shared<Shader>(SHZK_SPIRV_DIR "ibl_diffuse.spv", SHADER_FREQUENCY_COMPUTE, "main");
		m_resources[1].shader = std::make_shared<Shader>(SHZK_SPIRV_DIR "ibl_specular.spv", SHADER_FREQUENCY_COMPUTE, "main");

		RHIRootSignatureInfo info{};
		info.AddPushConstant({ .offset = 0, .size = 128, .frequency = SHADER_FREQUENCY_COMPUTE });
		info.AddEntry({ 0, 0, 1, SHADER_FREQUENCY_COMPUTE, RESOURCE_TYPE_COMBINED_IMAGE_SAMPLER })
			.AddEntry({ 0, 1, 1, SHADER_FREQUENCY_COMPUTE, RESOURCE_TYPE_RW_TEXTURE });
		m_resources[0].rs = RHI::Get()->CreateRootSignature(info);
		m_resources[0].descSet = m_resources[0].rs->CreateDescriptorSet(0);
		m_resources[1].rs = m_resources[0].rs;
		m_resources[1].descSet = m_resources[1].rs->CreateDescriptorSet(0);
		
		RHIComputePipelineInfo infoDiffuse{};
		infoDiffuse.computeShader = m_resources[0].shader->GetRHIShader();
		infoDiffuse.rootSignature = m_resources[0].rs;
		m_resources[0].pipeline = RHI::Get()->CreateComputePipeline(infoDiffuse);

		RHIComputePipelineInfo infoSpecular{};
		infoSpecular.computeShader = m_resources[1].shader->GetRHIShader();
		infoSpecular.rootSignature = m_resources[1].rs;
		m_resources[1].pipeline = RHI::Get()->CreateComputePipeline(infoSpecular);

		// TextureViews
		auto diffuseTex = RenderResourceManager::Get()->GetIBLDiffuseTexture();
		for (uint32_t f = 0; f < 6; ++f) 
		{
			RHITextureViewInfo v{};
			v.texture = diffuseTex; v.format = FORMAT_R16G16B16A16_SFLOAT;
			v.viewType = TextureViewType::View2D;
			v.subresourceRange = { TEXTURE_ASPECT_COLOR, 0, 1, f, 1 };
			m_diffuseFaceViews[f] = RHI::Get()->CreateTextureView(v);
		}
		auto specTex = RenderResourceManager::Get()->GetIBLSpecularTexture();
		for (uint32_t mip = 0; mip < m_specularMipCount; ++mip)
		{
			for (uint32_t face = 0; face < 6; ++face)
			{
				RHITextureViewInfo info{};
				info.texture = specTex; 
				info.format = FORMAT_R16G16B16A16_SFLOAT;
				info.viewType = TextureViewType::View2D;
				info.subresourceRange = { TEXTURE_ASPECT_COLOR, mip, 1, face, 1 };
				m_specularFaceViews.push_back(RHI::Get()->CreateTextureView(info));
			}
		}

		m_brdfLUT = std::make_shared<Texture>(SHZK_ASSETS_DIR "_builtin/BRDF_LUT.png", TextureType::Type2D, FORMAT_R8G8B8A8_UNORM);
	}

	void IBLPass::Prepare()
	{}

	void IBLPass::Execute(std::shared_ptr<RHICommandList> cmd)
	{
		if (!m_envMap || m_bEnvMapChanged) return;	// Execute when environment map changed
		m_bEnvMapChanged = false;

		RHIDescriptorUpdateInfo envInfo{};
		envInfo.binding = 0;
		envInfo.resourceType = RESOURCE_TYPE_COMBINED_IMAGE_SAMPLER;
		envInfo.textureView = m_envMap->GetRHITextureView();
		envInfo.sampler = RenderResourceManager::Get()->GetDefaultSampler()->GetRHISampler();
		m_resources[0].descSet->UpdateDescriptor(envInfo);
		m_resources[1].descSet->UpdateDescriptor(envInfo);

		cmd->TextureBarrier({ RenderResourceManager::Get()->GetIBLDiffuseTexture(),  RHIResourceState::Undefined,
	RHIResourceState::UnorderedAccess });
		cmd->TextureBarrier({ RenderResourceManager::Get()->GetIBLSpecularTexture(), RHIResourceState::Undefined,
	RHIResourceState::UnorderedAccess });

		for (uint32_t face = 0; face < 6; ++face) 
		{
			RHIDescriptorUpdateInfo info{}; 
			info.binding = 1;
			info.resourceType = RESOURCE_TYPE_RW_TEXTURE;
			info.textureView = m_diffuseFaceViews[face];
			m_resources[0].descSet->UpdateDescriptor(info);

			m_setting.front = m_fronts[face]; m_setting.up = m_ups[face];
			cmd->SetComputePipeline(m_resources[0].pipeline);
			cmd->BindDescriptorSet(m_resources[0].descSet, 0);
			cmd->PushConstants(&m_setting, sizeof(IBLSetting), SHADER_FREQUENCY_COMPUTE);
			cmd->Dispatch(32 / 16, 32 / 16, 1);
		}

		for (uint32_t mip = 0; mip < m_specularMipCount; ++mip)
		{
			for (uint32_t face = 0; face < 6; ++face) {
				RHIDescriptorUpdateInfo info{};
				info.binding = 1;
				info.resourceType = RESOURCE_TYPE_RW_TEXTURE;
				info.textureView = m_specularFaceViews[mip * 6 + face];
				m_resources[1].descSet->UpdateDescriptor(info);

				m_setting.front = m_fronts[face];
				m_setting.up = m_ups[face];
				m_setting.roughness = (float)mip / (float)(m_specularMipCount - 1);
				m_setting.mip = mip;
				cmd->SetComputePipeline(m_resources[1].pipeline);
				cmd->BindDescriptorSet(m_resources[1].descSet, 0);
				cmd->PushConstants(&m_setting, sizeof(IBLSetting), SHADER_FREQUENCY_COMPUTE);
				uint32_t size = 128u >> mip;
				cmd->Dispatch((size + 15) / 16, (size + 15) / 16, 1);
			}
		}
			

		cmd->TextureBarrier({ RenderResourceManager::Get()->GetIBLDiffuseTexture(),  RHIResourceState::UnorderedAccess,
	RHIResourceState::ShaderResource });
		cmd->TextureBarrier({ RenderResourceManager::Get()->GetIBLSpecularTexture(), RHIResourceState::UnorderedAccess,
	RHIResourceState::ShaderResource });
	}
}