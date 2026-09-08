#include "Material.h"
#include "runtime/asset/AssetManager.h"
#include "runtime/render/resources/Buffer.h"
#include "runtime/render/resources/RenderResourceManager.h"
#include "runtime/rhi/RHIResource.h"
#include "runtime/rhi/RHIDefinitions.h"

namespace shzk
{
	Material::Material()
		: Asset(AssetType::Material)
	{
		InitRenderResources();
	
		m_baseColor = glm::vec4(1.f);
		m_emission = glm::vec3(0.f);
		m_metallic = 0.f;
		m_roughness = 1.f;
		m_alphaCutoff = 0.5f;
		m_bUseVertexColor = false;
		for (int i = 0; i < 8; ++i)
		{
			m_ints[i] = 0;
			m_floats[i] = 0.f;
			m_colors[i] = glm::vec4(0.f);
		}
		UpdateUniformData();
	}

	void Material::InitRenderResources()
	{
		m_buffer = std::make_shared<Buffer<MaterialUniformShaderParameters>>();
		m_descriptorSet = RenderResourceManager::Get()->CreateMaterialDescriptorSet();
		if (!m_descriptorSet)
		{
			SHZK_LOG_ERROR("Material DescriptorSet Init Failed, check RenderResourceManager Init!");
			assert(false);
			return;
		}

		RHIDescriptorUpdateInfo info{};
		info.binding		= MATERIAL_BINDING_UNIFORM;
		info.index			= 0;
		info.resourceType = RESOURCE_TYPE_UNIFORM_BUFFER;
		info.buffer			= m_buffer->GetBuffer();		
		info.bufferOffset	= 0;
		info.bufferRange	= sizeof(MaterialUniformShaderParameters);
		m_descriptorSet->UpdateDescriptor(info);

		SetTextureBaseColor(AssetManager::GetWhiteTexture1x1());
		SetTextureArm(AssetManager::GetWhiteTexture1x1());
		SetTextureNormal(AssetManager::GetNormalTexture1x1());
		SetTextureOcclusion(AssetManager::GetWhiteTexture1x1());
		SetTextureEmissive(AssetManager::GetWhiteTexture1x1());
	}

	void Material::UpdateUniformData()
	{
		MaterialUniformShaderParameters ub;
		ub.baseColor = m_baseColor;
		ub.emission = m_emission;
		ub.metallic = m_metallic;
		ub.roughness = m_roughness;
		ub.alphaCutoff = m_alphaCutoff;
		ub.bUseVertexColor = m_bUseVertexColor ? 1 : 0;
		for (int i = 0; i < 8; ++i)
		{
			ub.ints[i] = m_ints[i];
			ub.floats[i] = m_floats[i];
			ub.colors[i] = m_colors[i];
		}
		m_buffer->SetData(ub);
	}

	void Material::UpdateTexture(uint32_t binding, std::shared_ptr<Texture> texture)
	{
		if (!texture || !texture->GetRHITextureView() || !m_descriptorSet) 
		{
			SHZK_LOG_WARN("Material::UpdateTexture failed, resources not adequate!");
			return;
		}

		RHIDescriptorUpdateInfo info{};
		info.binding		= binding;
		info.index			= 0;
		info.resourceType	= RESOURCE_TYPE_COMBINED_IMAGE_SAMPLER;
		info.textureView	= texture->GetRHITextureView();
		info.sampler		= RenderResourceManager::Get()->GetDefaultSampler()->GetRHISampler();
		m_descriptorSet->UpdateDescriptor(info);
	}
}