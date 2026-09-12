#include "RenderSystem.h"
#include "runtime/log/Log.h"
#include "runtime/global/Engine.h"
#include "runtime/window/WindowSystem.h"
#include "runtime/rhi/RHIDefinitions.h"
#include "runtime/rhi/RHI.h"
#include "runtime/rhi/RHICommandList.h"
#include "runtime/render/SceneRenderer.h"
#include "runtime/render/resources/RenderResourceManager.h"

#include "runtime/render/passes/RenderPass.h"
#include "runtime/render/passes/MeshPass.h"
#include "runtime/render/passes/DepthPrePass.h"
#include "runtime/render/passes/ForwardPass.h"
#include "runtime/render/passes/SkyPass.h"
#include "runtime/render/passes/PostProcessPass.h"
#include "runtime/render/passes/IBLPass.h"
#include "RenderConfig.h"
#include <cassert>

namespace shzk
{
// --- public functions ---
	void RenderSystem::Init()
	{
		InitRHIFundamentals();
		InitPerFrameRHIResources();
		InitPasses();

		m_sceneRenderer = std::make_shared<SceneRenderer>();
	}

	void RenderSystem::Shutdown()
	{	
		m_rhiGraphicsQueue->WaitIdle();
		for (auto& resource : m_perFrameResources)
		{
			resource.fence->Destroy();
			resource.startSemaphore->Destroy();
			resource.endSemaphore->Destroy();
			resource.cmdContext->Destroy();
		}

		m_rhiCmdPool->Destroy();
		m_rhiSwapchain->Destroy();
		m_rhiSurface->Destroy();
	}

	void RenderSystem::Tick()
	{
		RenderResourceManager::Get()->BeginFrame(m_currentFrameIndex);	// update frame index in RenderResourceManager

		PerFrameRHIResource& resource = m_perFrameResources[m_currentFrameIndex];
		resource.fence->Wait();

		std::shared_ptr<RHITexture> currentSwapchainTexture = m_rhiSwapchain->AcquireNextTexture(nullptr, resource.startSemaphore);
		
		m_sceneRenderer->Process(Engine::Get()->GetActiveScene());

		auto& cmd = RHICommandList::Get();
		cmd->SetContext(resource.cmdContext.get());
		cmd->BeginCommand();

		for (auto& pass : m_passes)
		{
			if (!pass) continue;
			pass->Prepare();
			pass->Execute(cmd);
		}
		
		std::shared_ptr<RHITexture> sceneColorTexture = RenderResourceManager::Get()->GetCurrentSceneColorTexture();
		std::shared_ptr<RHITextureView> sceneColorTextureView = RenderResourceManager::Get()->GetCurrentSceneColorTextureView();

		cmd->TextureBarrier({ sceneColorTexture, RHIResourceState::Common, RHIResourceState::TransferSrc });
		cmd->TextureBarrier({ currentSwapchainTexture, RHIResourceState::Undefined, RHIResourceState::TransferDst });
		cmd->CopyTexture(sceneColorTexture, sceneColorTexture->GetDefaultSubresourceLayers(), currentSwapchainTexture, currentSwapchainTexture->GetDefaultSubresourceLayers());
		//cmd->BlitTexture(sceneColorTexture, currentSwapchainTexture, sceneColorTexture->GetDefaultSubresourceLayers(), sceneColorTexture->GetDefaultSubresourceLayers(), FilterType::Linear);
		cmd->TextureBarrier({ currentSwapchainTexture, RHIResourceState::TransferDst, RHIResourceState::Present });

		cmd->EndCommand();

		cmd->Submit(resource.fence, resource.startSemaphore, resource.endSemaphore);
		m_rhiSwapchain->Present(resource.endSemaphore);

		m_currentFrameIndex = (m_currentFrameIndex + 1) % FRAMES_IN_FLIGHT;
	}

	// --- private functions ---
	void RenderSystem::InitRHIFundamentals()
	{
		m_rhiSurface = RHI::Get()->CreateSurface(Engine::GetWindowSystem()->GetWindow());
		assert(m_rhiSurface);

		RHIQueueInfo graphicsQueueInfo{};
		graphicsQueueInfo.type = RHIQueueType::Graphics;
		graphicsQueueInfo.index = 0;
		m_rhiGraphicsQueue = RHI::Get()->GetQueue(graphicsQueueInfo);
		assert(m_rhiGraphicsQueue);
		// RHIQueueInfo computeQueueInfo{};
		// computeQueueInfo.type = RHIQueueType::Compute;
		// computeQueueInfo.index = 0;
		// m_rhiComputeQueue = m_rhi->GetQueue(computeQueueinfo);
		// assert(m_rhiComputeQueue);

		RHISwapchainInfo swapchainInfo{};
		swapchainInfo.surface = m_rhiSurface;
		swapchainInfo.presentQueue = m_rhiGraphicsQueue;
		swapchainInfo.imageCount = FRAMES_IN_FLIGHT;
		swapchainInfo.extent = m_rhiSurface->GetExetent();
		swapchainInfo.format = COLOR_FORMAT;
		m_rhiSwapchain = RHI::Get()->CreateSwapchain(swapchainInfo);

		RenderResourceManager::Get()->SetRenderExtent(swapchainInfo.extent);

		RHICommandPoolInfo cmdPoolInfo{};
		cmdPoolInfo.queue = m_rhiGraphicsQueue;
		m_rhiCmdPool = RHI::Get()->CreateCommandPool(cmdPoolInfo);
		assert(m_rhiCmdPool);

		RHICommandList::Init(true);				// bypass = true
	}

	void RenderSystem::InitPerFrameRHIResources()
	{
		for (int i = 0; i < FRAMES_IN_FLIGHT; ++i)
		{
			m_perFrameResources[i].cmdContext		= m_rhiCmdPool->CreateCommandContext();
			m_perFrameResources[i].startSemaphore	= RHI::Get()->CreateSemaphore();
			m_perFrameResources[i].endSemaphore		= RHI::Get()->CreateSemaphore();
			m_perFrameResources[i].fence			= RHI::Get()->CreateFence();
		}
	}

	void RenderSystem::InitPasses()
	{
		// MeshPasses
		m_meshPasses[(size_t)PassType::DepthPre] = std::make_shared<DepthPrePass>();
		m_meshPasses[(size_t)PassType::Forward] = std::make_shared<ForwardPass>();
		m_meshPasses[(size_t)PassType::Sky] = std::make_shared<SkyPass>();

		m_meshPasses[(size_t)PassType::DepthPre]->Init();
		m_meshPasses[(size_t)PassType::Forward]->Init();
		m_meshPasses[(size_t)PassType::Sky]->Init();

		m_passes[(size_t)PassType::DepthPre] = m_meshPasses[(size_t)PassType::DepthPre];
		m_passes[(size_t)PassType::Forward] = m_meshPasses[(size_t)PassType::Forward];
		m_passes[(size_t)PassType::Sky] = m_meshPasses[(size_t)PassType::Sky];
		
		// Other Render Passes
		m_passes[(size_t)PassType::PostProcess] = std::make_shared<PostProcessPass>();
		m_passes[(size_t)PassType::PostProcess]->Init();
		m_passes[(size_t)PassType::IBL] = std::make_shared<IBLPass>();
		m_passes[(size_t)PassType::IBL]->Init();
		
	}
}