#pragma once

#include <string>
#include <memory>
#include <vector>

namespace shzk
{
	class Node;
	class CameraComponent;
	class SkyLightComponent;

	class Scene
	{
	public:
		Scene() = default;
		~Scene() = default;

		void Tick(float dt);
		void Clear();

		std::vector<std::shared_ptr<Node>> GetNodes() { return m_nodes; }
		std::shared_ptr<Node> GetNodeByName(const std::string& name) const;
		std::shared_ptr<Node> GetNodeById(uint32_t id) const;

		std::shared_ptr<CameraComponent> GetActiveCamera();
		std::shared_ptr<SkyLightComponent> GetActiveSkyLight();

		void AddNode(std::shared_ptr<Node> node);

	private:
		std::string m_name;
		std::vector<std::shared_ptr<Node>> m_nodes;
	};
}