#include "SceneRenderer.h"
#include "MeshDrawCommand.h"

#include "runtime/log/Log.h"
#include "runtime/global/Engine.h"
#include "runtime/render/MeshBatch.h"
#include "runtime/render/RenderSystem.h"
#include "runtime/render/passes/MeshPass.h"
#include "runtime/render/passes/IBLPass.h"
#include "runtime/render/passes/MeshPassProcessor.h"
#include "runtime/render/resources/RenderResourceManager.h"
#include "runtime/framework/Scene.h"
#include "runtime/framework/Node.h"
#include "runtime/framework/components/MeshComponent.h"
#include "runtime/framework/components/TransformComponent.h"
#include "runtime/framework/components/CameraComponent.h"
#include "runtime/framework/components/SkyBoxComponent.h"
#include "runtime/framework/components/SkyLightComponent.h"

#include <glm/glm.hpp>
#include <cassert>

namespace shzk
{
	void SceneRenderer::Process(std::shared_ptr<Scene> scene)
	{
		// Init Views
		InitActiveCameraView(scene);
		InitSkyLight(scene);

		// CollectBatches
		std::vector<MeshBatch>	batches;
		glm::mat4 transformMat = glm::identity<glm::mat4>();

		const auto& nodes = scene->GetNodes();
		for (const auto& node : nodes)	CollectNodeMesh(node, batches, transformMat);

		auto& passes = Engine::GetRenderSystem()->GetMeshPasses();
		for (auto& pass : passes)
		{
			if (!pass) continue;
			pass->GetMeshPassProcessor()->Process(batches);
		}
	}

	void SceneRenderer::InitActiveCameraView(std::shared_ptr<Scene> scene)
	{
		PerFrameUniformShaderParameters params{};

		std::shared_ptr<CameraComponent> cameraComp = scene->GetActiveCamera();
		if (!cameraComp) 
		{
			SHZK_LOG_ERROR("No active camera in active scene!");
			assert(false);
			return;
		}
		
		Extent2D extent = RenderResourceManager::Get()->GetRenderExtent();
		float aspect = (float)extent.width / extent.height;
		
		params.view = cameraComp->GetViewMatrix();
		params.proj = cameraComp->GetProjMatrix(aspect);
		params.viewProj = params.proj * params.view;
		
		std::shared_ptr<Buffer<PerFrameUniformShaderParameters>> buffer = RenderResourceManager::Get()->GetCurrentPerFrameUniformBuffer();
		buffer->SetData(params);
	}

	void SceneRenderer::InitSkyLight(std::shared_ptr<Scene> scene)
	{
		auto skyLight = scene->GetActiveSkyLight();
		if (!skyLight || !skyLight->GetEnvironmentMap()) return;

		auto iblPass = Engine::GetRenderSystem()->GetPass<IBLPass>(PassType::IBL);
		if (skyLight->GetEnvironmentMap() != iblPass->GetEnvMap())
		{
			iblPass->SetEnvMap(skyLight->GetEnvironmentMap());
		}	
	}

	void SceneRenderer::CollectNodeMesh(const std::shared_ptr<Node>& node, std::vector<MeshBatch>& batches, glm::mat4x4 accTransformMat)
	{
		std::shared_ptr<TransformComponent> transformComp = node->TryGetComponent<TransformComponent>();
		if (!transformComp) return;

		accTransformMat = transformComp->GetTransform().ToMat4x4() * accTransformMat;

		std::shared_ptr<MeshComponent>	meshComp = node->TryGetComponent<MeshComponent>();
		if (meshComp)
		{
			meshComp->CollectMeshBatchWithTransform(batches, accTransformMat);
		}

		std::shared_ptr<SkyBoxComponent> skyBoxComp = node->TryGetComponent<SkyBoxComponent>();
		if (skyBoxComp)
		{
			skyBoxComp->CollectMeshBatchWithTransform(batches, accTransformMat);
		}

		// recursive
		for (const auto& child : node->GetChildren())
		{		 
			CollectNodeMesh(child, batches, accTransformMat);
		}
	}
}