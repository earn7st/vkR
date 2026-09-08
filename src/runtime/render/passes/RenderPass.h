#pragma once

#include <cstdint>
#include <memory>

namespace shzk
{
	enum class PassType : uint32_t
	{
		IBL						= 0,
	
		DepthPre				= 1,
		Forward					= 2,
		Sky						= 3,
		DirectionalShadowMap	= 4,
		PointLightShadowMap		= 5,

		PostProcess				= 6,

		Max,
	};

	enum class MeshPassType : uint32_t
	{
		DepthPre = 0,
		Forward = 1,
		Sky = 2,
		DirectionalShadowMap = 3,
		PointLightShadowMap = 4,

		Max,
	};

	class RHICommandList;

	class RenderPass
	{
	public:
		RenderPass() = delete;
		RenderPass(PassType type) : m_type(type) {}
		~RenderPass() = default;

		virtual void Init() = 0;
		virtual void Prepare() = 0;
		virtual void Execute(std::shared_ptr<RHICommandList> cmd) = 0;

		PassType GetType() const { return m_type; }

	protected:
		PassType	m_type = PassType::Max;
		bool		m_enabled = true;
	};
}