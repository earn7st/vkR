
// #define _CRTDBG_MAP_ALLOC
// #include <crtdbg.h>
#include <memory>
#include "editor/Editor.h"
#include "runtime/core/Transform.h"
#include "runtime/asset/AssetManager.h"
#include "runtime/asset/Texture.h"
#include "runtime/asset/Material.h"
#include "runtime/global/Engine.h"
// framework
#include "runtime/framework/Scene.h"
#include "runtime/framework/Node.h"
#include "runtime/framework/components/MeshComponent.h"
#include "runtime/framework/components/CameraComponent.h"	
#include "runtime/framework/components/TransformComponent.h"	
#include "runtime/framework/components/SkyBoxComponent.h"
#include "runtime/framework/components/SkyLightComponent.h"
//import
#include "runtime/import/GltfLoader.h"

void InitScene(std::shared_ptr<shzk::Scene>& scene)
{

	if (true)
	{
		// SkyBox
		std::shared_ptr<shzk::Node> skybox = std::make_shared<shzk::Node>(0, "skybox_climbing_gym");
		std::shared_ptr<shzk::TransformComponent> transformComp = std::make_shared<shzk::TransformComponent>();
		skybox->AddComponent(transformComp);
		std::shared_ptr<shzk::SkyBoxComponent> skyboxComp = std::make_shared<shzk::SkyBoxComponent>();
		skybox->AddComponent(skyboxComp);

		std::vector<std::string> restingPlacePaths = {
			SHZK_ASSETS_DIR "_builtin/environment/climbing_gym/CubeMap_4K/px.png",
			SHZK_ASSETS_DIR "_builtin/environment/climbing_gym/CubeMap_4K/nx.png",
			SHZK_ASSETS_DIR "_builtin/environment/climbing_gym/CubeMap_4K/py.png",
			SHZK_ASSETS_DIR "_builtin/environment/climbing_gym/CubeMap_4K/ny.png",
			SHZK_ASSETS_DIR "_builtin/environment/climbing_gym/CubeMap_4K/pz.png",
			SHZK_ASSETS_DIR "_builtin/environment/climbing_gym/CubeMap_4K/nz.png" };
		std::shared_ptr<shzk::Texture> restingPlaceCubeMap = std::make_shared<shzk::Texture>(restingPlacePaths, shzk::TextureType::TypeCube, shzk::RHIFormat::FORMAT_R8G8B8A8_SRGB);

		std::shared_ptr<shzk::Material> skyboxMaterial = skyboxComp->GetMaterial();
		skyboxMaterial->SetTextureCubeSlot(0, restingPlaceCubeMap);
		
		// Environment Map for SkyLight
		std::shared_ptr<shzk::SkyLightComponent> skyLight = std::make_shared<shzk::SkyLightComponent>();

		std::shared_ptr<shzk::Texture> skyLightHDR = std::make_shared<shzk::Texture>(SHZK_ASSETS_DIR "_builtin/environment/climbing_gym/climbing_gym_4k.hdr", shzk::TextureType::TypeEquirectangular, shzk::RHIFormat::FORMAT_R16G16B16A16_SFLOAT);
		skyLight->SetEnvironmentMap(skyLightHDR);
		skybox->AddComponent(skyLight);

		scene->AddNode(skybox);
	}

	// Damaged Helmet
	if (true)
	{
		shzk::GltfLoadResult helmetResult;
		shzk::GltfLoader::Get()->Load(SHZK_ASSETS_DIR "DamagedHelmet/glTF/DamagedHelmet.gltf", helmetResult);
		shzk::AssetManager::Get()->ProcessGltfLoadResult(helmetResult);

		std::shared_ptr<shzk::Node> helmet0 = std::make_shared<shzk::Node>(1, "damaged_helmet0");
		std::shared_ptr<shzk::Node> helmet1 = std::make_shared<shzk::Node>(2, "damaged_helmet1");
		
		shzk::Transform transform{};
		transform.SetTranslation(glm::vec3(1.f, 0.f, 0.f));
		{
			std::shared_ptr<shzk::TransformComponent> transformComp = std::make_shared<shzk::TransformComponent>();
			transformComp->SetTransform(transform);
			helmet0->AddComponent(transformComp);
			std::shared_ptr<shzk::MeshComponent> meshComp = std::make_shared<shzk::MeshComponent>();
			meshComp->SetModel(helmetResult.models[0]);
			helmet0->AddComponent(meshComp);
		}

		{
			std::shared_ptr<shzk::TransformComponent> transformComp = std::make_shared<shzk::TransformComponent>();
			transform.SetTranslation(glm::vec3(-1.f, 0.f, 0.f));
			transformComp->SetTransform(transform);
			helmet1->AddComponent(transformComp);
			std::shared_ptr<shzk::MeshComponent> meshComp = std::make_shared<shzk::MeshComponent>();
			meshComp->SetModel(helmetResult.models[0]);
			helmet1->AddComponent(meshComp);
		}
		
		scene->AddNode(helmet0);
		scene->AddNode(helmet1);
	}

	// Sponza
	if (false)
	{
		shzk::GltfLoadResult sponzaResult;
		shzk::GltfLoader::Get()->Load(SHZK_ASSETS_DIR "Sponza/glTF/Sponza.gltf", sponzaResult);
		shzk::AssetManager::Get()->ProcessGltfLoadResult(sponzaResult);

		std::shared_ptr<shzk::Node> sponza = std::make_shared<shzk::Node>(1, "sponza");
		shzk::Transform transform{};
		transform.SetTranslation(glm::vec3(0.f, 0.f, 0.f));

		{
			std::shared_ptr<shzk::TransformComponent> transformComp = std::make_shared<shzk::TransformComponent>();
			transformComp->SetTransform(transform);
			sponza->AddComponent(transformComp);
			std::shared_ptr<shzk::MeshComponent> meshComp = std::make_shared<shzk::MeshComponent>();
			meshComp->SetModel(sponzaResult.models[0]);
			sponza->AddComponent(meshComp);
		}

		scene->AddNode(sponza);
	}

}

int main()
{
	// _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_CHECK_ALWAYS_DF);

	shzk::EngineInitInfo engineInitInfo{};
	engineInitInfo.name = "shzk";
	engineInitInfo.width = 1920;
	engineInitInfo.height = 1080;

	shzk::Engine::Init(engineInitInfo);

	std::shared_ptr<shzk::Scene> scene = std::make_shared<shzk::Scene>();
	InitScene(scene);
	shzk::Engine::Get()->SetActiveScene(scene);

	shzk::Editor* editor = new shzk::Editor();
	editor->Init(scene);
	editor->Run();
	editor->Shutdown();

	return 0;
}