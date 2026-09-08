#pragma once
#include "Component.h"

#include <memory>

namespace shzk
{
	class Texture;

	class SkyLightComponent : public Component
	{
	public:
		SkyLightComponent() : Component(ComponentType::SkyLight) {};
		~SkyLightComponent() = default;

		void SetEnvironmentMap(std::shared_ptr<Texture> envMap) { m_envMap = envMap; }
		std::shared_ptr<Texture> GetEnvironmentMap() const { return m_envMap; }

	private:
		std::shared_ptr<Texture> m_envMap;
	};
}