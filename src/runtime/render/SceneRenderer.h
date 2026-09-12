#pragma once

#include <memory>
#include <vector>
#include <glm/glm.hpp>

namespace shzk
{
	class Scene;
	class Node;
	struct MeshBatch;

	class SceneRenderer
	{
	public:
		void Process(std::shared_ptr<Scene> scene);

	private:
		void InitActiveCameraView(std::shared_ptr<Scene> scene);
		// void InitViews();
		void InitSkyLight(std::shared_ptr<Scene> scene);
		void CollectNodeMesh(const std::shared_ptr<Node>& node, std::vector<MeshBatch>& batches, glm::mat4x4 accTransformMat);
	};
}