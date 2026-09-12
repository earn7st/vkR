#include "Scene.h"

#include "runtime/log/Log.h"
#include "runtime/framework/Node.h"
#include "runtime/framework/components/CameraComponent.h"
#include "runtime/framework/components/TransformComponent.h"
#include "runtime/framework/components/SkyLightComponent.h"

#include <cassert>

namespace shzk
{
	void Scene::Tick(float dt)
	{
		for (auto& node : m_nodes)
		{
			node->Tick(dt);
		}
	}

	void Scene::Clear()
	{
		m_nodes.clear();
	}

	std::shared_ptr<Node> Scene::GetNodeByName(const std::string& name) const
	{
		for (auto& node : m_nodes)
		{
			if (node->m_name == name)
			{
				return node;
			}
		}
		SHZK_LOG_WARN("Scene {} doesn't have Node which name is {}", m_name, name);
		return std::shared_ptr<Node>();
	}

	std::shared_ptr<Node> Scene::GetNodeById(uint32_t id) const
	{
		for (auto& node : m_nodes)
		{
			if (node->m_id == id)
			{
				return node;
			}
		}
		SHZK_LOG_WARN("Scene {} doesn't have Node which id is {}", m_name, id);
		return std::shared_ptr<Node>();
	}

	std::shared_ptr<CameraComponent> Scene::GetActiveCamera()
	{
		for (auto& node : m_nodes)
		{
			std::shared_ptr<CameraComponent> cameraComp = node->TryGetComponent<CameraComponent>();
			if (cameraComp && cameraComp->IsActiveCamera()) return cameraComp;
		}
		return nullptr;
	}

	std::shared_ptr<SkyLightComponent> Scene::GetActiveSkyLight()
	{
		for (auto& node : m_nodes)
		{
			std::shared_ptr<SkyLightComponent> skyLightComp = node->TryGetComponent<SkyLightComponent>();
			if (skyLightComp && skyLightComp->GetEnvironmentMap()) return skyLightComp;
		}
		return nullptr;
	}

	void Scene::AddNode(std::shared_ptr<Node> node)
	{
		assert(node);
		m_nodes.push_back(node);
		SHZK_LOG_INFO("Node {} added to Scene {}", node->m_name, m_name);
	}
}
