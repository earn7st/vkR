#pragma once

#include "RenderPass.h"

#include <memory>
#include <array>
#include <vector>
#include <glm/glm.hpp>

namespace shzk
{
	class Shader;
	class Texture;
	class RHICommandList;
	class RHIRootSignature;
	class RHIDescriptorSet;
	class RHIComputePipeline;
	class RHITextureView;

	class IBLPass : public RenderPass
	{
	public:
		IBLPass() : RenderPass(PassType::IBL) {}
		~IBLPass() = default;

		virtual void Init() override final;
		virtual void Prepare() override final;
		virtual void Execute(std::shared_ptr<RHICommandList> cmd) override final;

		inline void SetEnvMap(std::shared_ptr<Texture> envMap) { m_envMap = envMap; m_bEnvMapChanged = true; }
		inline std::shared_ptr<Texture> GetEnvMap() const { return m_envMap; }

	private:
		struct IBLPassResources
		{
			std::shared_ptr<Shader> shader;
			std::shared_ptr<RHIRootSignature> rs;
			std::shared_ptr<RHIDescriptorSet> descSet;
			std::shared_ptr<RHIComputePipeline> pipeline;
		};
		std::array<IBLPassResources, 2> m_resources; // 0: diffuse 1: specular

		struct IBLSetting 
		{
			glm::vec4 front, up;
			float deltaPhi		= (2.0f * 3.14159265f) / 180.0f;
			float deltaTheta	= (0.5f * 3.14159265f) / 64.0f;
			float roughness		= 0.0f;
			uint32_t numSamples = 32;
			uint32_t mip		= 0;
		};
		IBLSetting m_setting;

		std::array<std::shared_ptr<RHITextureView>, 6>	m_diffuseFaceViews;
		std::vector<std::shared_ptr<RHITextureView>>    m_specularFaceViews;  // [mip*6 + face] as index
		uint32_t m_specularMipCount = 5;
		std::vector<glm::vec4> m_fronts = { {1, 0, 0, 0}, {-1, 0, 0, 0}, {0, 1, 0, 0}, {0, -1, 0, 0}, {0, 0, 1, 0}, {0, 0, -1, 0} };
		std::vector<glm::vec4> m_ups = { {0, -1, 0, 0}, {0, -1, 0, 0}, {0, 0, 1, 0}, {0, 0, -1, 0}, {0, -1, 0, 0}, {0, -1, 0, 0} };

		std::shared_ptr<Texture> m_envMap;
		bool m_bEnvMapChanged = false;
		std::shared_ptr<Texture> m_brdfLUT;
	};
}