#include "RenderResourceManager.h"
#include "runtime/log/Log.h"
#include "runtime/global/Engine.h"
#include "runtime/window/WindowSystem.h"
#include "runtime/render/RenderConfig.h"
#include "runtime/render/resources/Sampler.h"
#include "runtime/rhi/RHI.h"
#include "runtime/rhi/RHIResource.h"
#include "runtime/rhi/RHIDefinitions.h"


#include <memory>
#include <fstream>

namespace shzk
{
	std::shared_ptr<RenderResourceManager> RenderResourceManager::g_renderResourceManager = std::make_shared<RenderResourceManager>();

    void RenderResourceManager::Init()
    {
        m_renderExtent = { Engine::GetWindowSystem()->GetWidth(), Engine::GetWindowSystem()->GetHeight() };
        InitGlobalResources();
    }

	std::shared_ptr<RHIShader> RenderResourceManager::GetOrCreateRHIShader(const std::string& path, ShaderFrequency frequency, const std::string& entry)
	{
		auto iter = m_rhiShaderMap.find(path);
		if (iter != m_rhiShaderMap.end()) return iter->second;

        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if (!file.is_open())
        {
            SHZK_LOG_ERROR("Failed to open shader file: {}", path);
            return nullptr;
        }

        std::streamsize fileSize = file.tellg();
        file.seekg(0, std::ios::beg);

        std::vector<uint8_t> code(static_cast<size_t>(fileSize));
        if (!code.empty())
            file.read(reinterpret_cast<char*>(code.data()), fileSize);

        RHIShaderInfo shaderInfo = {
            .entry = entry,
            .frequency = frequency,
            .code = std::move(code),
        };

        std::shared_ptr<RHIShader> shader = RHI::Get()->CreateShader(shaderInfo);

        m_rhiShaderMap.emplace(path, shader);
        return shader;
	}

    void RenderResourceManager::BeginFrame(uint32_t frameIdx)
    {
        m_frameIndex = frameIdx;
    }

    std::shared_ptr<RHIDescriptorSet> RenderResourceManager::CreateMaterialDescriptorSet()
    {
        return m_materialRootSignature->CreateDescriptorSet(DESCRIPTORSET_INDEX_MATERIAL);
    }

    void RenderResourceManager::InitGlobalResources()
    {
        // per frame root signature
        {
            RHIRootSignatureInfo perFrameInfo{};
            perFrameInfo.AddEntry({ .set = DESCRIPTORSET_INDEX_PER_FRAME, .binding = PER_FRAME_BINDING_VIEW, .size = 1, .frequency = SHADER_FREQUENCY_ALL, .type = RESOURCE_TYPE_UNIFORM_BUFFER });
            m_perFrameRootSignature = RHI::Get()->CreateRootSignature(perFrameInfo);
        }
        
        // material root signature
        {
            RHIRootSignatureInfo matInfo{};
            matInfo.AddEntry({ .set = DESCRIPTORSET_INDEX_MATERIAL, .binding = MATERIAL_BINDING_UNIFORM, .size = 1, .frequency = SHADER_FREQUENCY_FRAGMENT, .type = RESOURCE_TYPE_UNIFORM_BUFFER })
                .AddEntry({ .set = DESCRIPTORSET_INDEX_MATERIAL, .binding = MATERIAL_BINDING_BASECOLOR, .size = 1, .frequency = SHADER_FREQUENCY_FRAGMENT, .type = RESOURCE_TYPE_COMBINED_IMAGE_SAMPLER })
                .AddEntry({ .set = DESCRIPTORSET_INDEX_MATERIAL, .binding = MATERIAL_BINDING_ARM, .size = 1, .frequency = SHADER_FREQUENCY_FRAGMENT, .type = RESOURCE_TYPE_COMBINED_IMAGE_SAMPLER })
                .AddEntry({ .set = DESCRIPTORSET_INDEX_MATERIAL, .binding = MATERIAL_BINDING_NORMAL, .size = 1, .frequency = SHADER_FREQUENCY_FRAGMENT, .type = RESOURCE_TYPE_COMBINED_IMAGE_SAMPLER })
                .AddEntry({ .set = DESCRIPTORSET_INDEX_MATERIAL, .binding = MATERIAL_BINDING_OCCLUSION, .size = 1, .frequency = SHADER_FREQUENCY_FRAGMENT, .type = RESOURCE_TYPE_COMBINED_IMAGE_SAMPLER })
                .AddEntry({ .set = DESCRIPTORSET_INDEX_MATERIAL, .binding = MATERIAL_BINDING_EMISSIVE, .size = 1, .frequency = SHADER_FREQUENCY_FRAGMENT, .type = RESOURCE_TYPE_COMBINED_IMAGE_SAMPLER });
            for (int i = 0; i < 4; ++i)
                matInfo.AddEntry({ .set = DESCRIPTORSET_INDEX_MATERIAL, .binding = MATERIAL_BINDING_TEXTURE2D + i, .size = 1, .frequency = SHADER_FREQUENCY_FRAGMENT, .type = RESOURCE_TYPE_COMBINED_IMAGE_SAMPLER });
            for (int i = 0; i < 4; ++i)
                matInfo.AddEntry({ .set = DESCRIPTORSET_INDEX_MATERIAL, .binding = MATERIAL_BINDING_TEXTURECUBE + i, .size = 1, .frequency = SHADER_FREQUENCY_FRAGMENT, .type = RESOURCE_TYPE_COMBINED_IMAGE_SAMPLER });
            for (int i = 0; i < 4; ++i)
                matInfo.AddEntry({ .set = DESCRIPTORSET_INDEX_MATERIAL, .binding = MATERIAL_BINDING_TEXTURE3D + i, .size = 1, .frequency = SHADER_FREQUENCY_FRAGMENT, .type = RESOURCE_TYPE_COMBINED_IMAGE_SAMPLER });
            m_materialRootSignature = RHI::Get()->CreateRootSignature(matInfo);
        }
        
        // per frame resources
        {
            for (int i = 0; i < FRAMES_IN_FLIGHT; ++i)
            {
                auto& perFrame = m_perFrameResources[i];
                // descriptor set (set 0)
                perFrame.descriptorSet = m_perFrameRootSignature->CreateDescriptorSet(DESCRIPTORSET_INDEX_PER_FRAME);
                perFrame.ub = std::make_shared<Buffer<PerFrameUniformShaderParameters>>();

				RHIDescriptorUpdateInfo info{};
				info.binding        = PER_FRAME_BINDING_VIEW;
				info.index          = 0;
				info.resourceType   = RESOURCE_TYPE_UNIFORM_BUFFER;
                info.buffer         = perFrame.ub->GetBuffer();
                info.bufferOffset   = 0;
				info.bufferRange    = sizeof(PerFrameUniformShaderParameters);
                perFrame.descriptorSet->UpdateDescriptor(info);

                // HDR color attachment
                RHITextureInfo hdrColorInfo{};
                hdrColorInfo.format = HDR_COLOR_FORMAT;
                hdrColorInfo.extent = { m_renderExtent.width, m_renderExtent.height, 1 };
                hdrColorInfo.arrayLayers = 1;
                hdrColorInfo.mipLevels = 1;
                hdrColorInfo.memoryUsage = MemoryUsage::GPUOnly;
                hdrColorInfo.type = RESOURCE_TYPE_TEXTURE | RESOURCE_TYPE_RENDER_TARGET | RESOURCE_TYPE_RW_TEXTURE;
                perFrame.hdrColorTexture = RHI::Get()->CreateTexture(hdrColorInfo);

                RHITextureViewInfo hdrViewInfo{};
                hdrViewInfo.texture = perFrame.hdrColorTexture;
                hdrViewInfo.format = HDR_COLOR_FORMAT;
                hdrViewInfo.viewType = TextureViewType::View2D;
                perFrame.hdrColorTextureView = RHI::Get()->CreateTextureView(hdrViewInfo);
                

                // color attachment
                RHITextureInfo colorInfo{};
                colorInfo.format = COLOR_FORMAT;
                colorInfo.extent = { m_renderExtent.width, m_renderExtent.height, 1 };
                colorInfo.arrayLayers = 1;
                colorInfo.mipLevels = 1;
                colorInfo.memoryUsage = MemoryUsage::GPUOnly;
                colorInfo.type = RESOURCE_TYPE_TEXTURE | RESOURCE_TYPE_RENDER_TARGET | RESOURCE_TYPE_RW_TEXTURE;
                perFrame.sceneColorTexture = RHI::Get()->CreateTexture(colorInfo);

                RHITextureViewInfo viewInfo{};
                viewInfo.texture = perFrame.sceneColorTexture;
                viewInfo.format = COLOR_FORMAT;
                viewInfo.viewType = TextureViewType::View2D;
                perFrame.sceneColorTextureView = RHI::Get()->CreateTextureView(viewInfo);
                
                // depth attachment
                {
                    RHITextureInfo depthInfo{};
                    depthInfo.format = DEPTH_FORMAT;
                    depthInfo.extent = { m_renderExtent.width, m_renderExtent.height, 1 };
                    depthInfo.arrayLayers = 1;
                    depthInfo.mipLevels = 1;
                    depthInfo.memoryUsage = MemoryUsage::GPUOnly;
                    depthInfo.type = RESOURCE_TYPE_TEXTURE;
                    perFrame.sceneDepthTexture = RHI::Get()->CreateTexture(depthInfo);

                    RHITextureViewInfo depthViewInfo{};
                    depthViewInfo.texture = perFrame.sceneDepthTexture;
                    depthViewInfo.format = DEPTH_FORMAT;
                    depthViewInfo.viewType = TextureViewType::View2D;
                    perFrame.sceneDepthTextureView = RHI::Get()->CreateTextureView(depthViewInfo);
                }
            }
        }

        // multiframe resources
        {
            // default sampler
            m_samplers.push_back(std::make_shared<Sampler>(
                FilterType::Linear,
                SamplerMipmapMode::Linear,
                SamplerAddressMode::Repeat,
                0.f));

            // IBL
            {
                const uint32_t IRR_SIZE = 32, SPEC_SIZE = 128, SPEC_MIPS = 5;

                RHITextureInfo irr{};
                irr.format = FORMAT_R16G16B16A16_SFLOAT;
                irr.extent = { IRR_SIZE, IRR_SIZE, 1 };
                irr.arrayLayers = 6;
                irr.mipLevels = 1;
                irr.memoryUsage = MemoryUsage::GPUOnly;
                irr.type = RESOURCE_TYPE_TEXTURE | RESOURCE_TYPE_TEXTURE_CUBE | RESOURCE_TYPE_RW_TEXTURE;
                m_iblDiffuse = RHI::Get()->CreateTexture(irr);

                RHITextureViewInfo irrV{};
                irrV.texture = m_iblDiffuse; irrV.format = irr.format;
                irrV.viewType = TextureViewType::ViewCube;
                irrV.subresourceRange = { TEXTURE_ASPECT_COLOR, 0, 1, 0, 6 };
                m_iblDiffuseView = RHI::Get()->CreateTextureView(irrV);

                RHITextureInfo spec{};
                spec.format = FORMAT_R16G16B16A16_SFLOAT;
                spec.extent = { SPEC_SIZE, SPEC_SIZE, 1 };
                spec.arrayLayers = 6;
                spec.mipLevels = SPEC_MIPS;
                spec.memoryUsage = MemoryUsage::GPUOnly;
                spec.type = RESOURCE_TYPE_TEXTURE | RESOURCE_TYPE_TEXTURE_CUBE | RESOURCE_TYPE_RW_TEXTURE;
                m_iblSpecular = RHI::Get()->CreateTexture(spec);

                RHITextureViewInfo specV{};
                specV.texture = m_iblSpecular; specV.format = spec.format;
                specV.viewType = TextureViewType::ViewCube;
                specV.subresourceRange = { TEXTURE_ASPECT_COLOR, 0, SPEC_MIPS, 0, 6 };
                m_iblSpecularView = RHI::Get()->CreateTextureView(specV);
            }
        }
        
    }
}