#pragma once 

#include "RenderPass.h"	
#include "runtime/rhi/RHIDefinitions.h"

namespace shzk
{
	class RHIComandList;
	class RHIComputePipeline;
	class RHIRootSignature;
	class RHIDescriptorSet;
	class Shader;

	class PostProcessPass : public RenderPass
	{
	public:
		PostProcessPass() : RenderPass(PassType::PostProcess) {}
		~PostProcessPass() = default;

		virtual void Init() override final;
		virtual void Prepare() override final;
		virtual void Execute(std::shared_ptr<RHICommandList> cmd) override final;

	private:
		// TODO: ProcessProcess Setting Uniform Buffer
		/*struct PostProcessingSetting
		{
			float exposure = 0.4f;
			float luminance = 0.1f;
			float saturation = 1.0f;
			float contrast = 1.0f;
			uint32_t mode = 2;
			uint32_t fixLuminance = 0;
		};
		PostProcessingSetting setting;*/

		std::shared_ptr<Shader> m_shader;
		std::shared_ptr<RHIRootSignature> m_rootSignature;
		std::array<std::shared_ptr<RHIDescriptorSet>, FRAMES_IN_FLIGHT> m_descriptorSets;
		std::shared_ptr<RHIComputePipeline> m_pipeline;
	};
}