#include "Texture.h"
#include "Asset.h"
#include "runtime/log/Log.h"
#include "runtime/rhi/RHI.h"
#include "runtime/rhi/RHIUtil.h"
#include "runtime/rhi/RHIDefinitions.h"
#include "runtime/rhi/RHIResource.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <filesystem>
#include <memory>
#include <glm/glm.hpp>

namespace shzk
{
	static uint16_t Float32ToFloat16(float value)
	{
		uint32_t bits;
		memcpy(&bits, &value, sizeof(bits));

		uint32_t sign = (bits >> 16) & 0x8000u;
		int32_t  exp = (int32_t)((bits >> 23) & 0xFFu) - 127 + 15;
		uint32_t mant = bits & 0x7FFFFFu;

		if (exp <= 0)
		{
			if (exp < -10) 
				return (uint16_t)sign;
			mant |= 0x800000u;
			return (uint16_t)(sign | (mant >> (14 - exp)));
		}
		if (exp >= 31)
			return (uint16_t)(sign | 0x7C00u);

		return (uint16_t)(sign | ((uint32_t)exp << 10) | (mant >> 13));
	}

	TextureViewType TextureTypeToTextureViewType(TextureType type)
	{
		TextureViewType viewType;
		switch (type) {
		case TextureType::Type2D:           viewType = TextureViewType::View2D;       break;
		case TextureType::Type2DArray:      viewType = TextureViewType::View2DArray;  break;
		case TextureType::TypeCube:         viewType = TextureViewType::ViewCube;     break;
		case TextureType::Type3D:           viewType = TextureViewType::View3D;       break;
		case TextureType::TypeEquirectangular: viewType = TextureViewType::View2D;    break;
		default:							SHZK_LOG_ERROR("Unsupported texture type!");
		}
		return viewType;
	}

	Texture::Texture(std::string path, TextureType type, RHIFormat format)
		: Asset(AssetType::Texture), m_type(type), m_format(format), m_arrayLayer(1)
	{
		m_paths.push_back(path);
		if (format == RHIFormat::FORMAT_R16G16B16A16_SFLOAT || format == RHIFormat::FORMAT_R32G32B32A32_SFLOAT)
		{
			LoadFromFileHDR();
		}
		else
		{
			LoadFromFile();
		}
	}

	Texture::Texture(const std::vector<std::string>& paths, TextureType type, RHIFormat format)
		: Asset(AssetType::Texture), m_type(type), m_format(format), m_arrayLayer((uint32_t)paths.size())
	{
		m_paths = paths;
		LoadFromFile();
	}

	Texture::Texture(Extent2D extent, glm::vec4 rgba, RHIFormat format)
		: Asset(AssetType::Texture),
		m_type(TextureType::Type2D),
		m_format(format),
		m_extent({ extent.width, extent.height, 1 }),
		m_mipLevels(1),
		m_arrayLayer(1)
	{
		InitRHI();

		const uint32_t pixelCount = extent.width * extent.height;
		const uint32_t bufferSize = pixelCount * 4;
		std::vector<uint8_t> pixels(bufferSize);

		const uint8_t r = (uint8_t)(glm::clamp(rgba.r, 0.0f, 1.0f) * 255.0f + 0.5f);
		const uint8_t g = (uint8_t)(glm::clamp(rgba.g, 0.0f, 1.0f) * 255.0f + 0.5f);
		const uint8_t b = (uint8_t)(glm::clamp(rgba.b, 0.0f, 1.0f) * 255.0f + 0.5f);
		const uint8_t a = (uint8_t)(glm::clamp(rgba.a, 0.0f, 1.0f) * 255.0f + 0.5f);
		for (uint32_t i = 0; i < pixelCount; ++i)
		{
			pixels[i * 4 + 0] = r;
			pixels[i * 4 + 1] = g;
			pixels[i * 4 + 2] = b;
			pixels[i * 4 + 3] = a;
		}

		auto immediateCmd = RHI::Get()->GetCommandContextImmediate();

		RHIBufferInfo bufferInfo = {
			.size = bufferSize,
			.memoryUsage = MemoryUsage::CPUOnly,
			.type = RESOURCE_TYPE_BUFFER,
			.creationFlag = BUFFER_CREATION_PERSISTENT_MAP
		};
		std::shared_ptr<RHIBuffer> stagingBuffer = RHI::Get()->CreateBuffer(bufferInfo);
		memcpy(stagingBuffer->Map(), pixels.data(), bufferSize);

		immediateCmd->RHITextureBarrierCommand(
			{ m_texture, RHIResourceState::Undefined, RHIResourceState::TransferDst,
			  { TEXTURE_ASPECT_COLOR, 0, 1, 0, 1 } });
		immediateCmd->RHICopyBufferToTexture(stagingBuffer, 0, m_texture, { TEXTURE_ASPECT_COLOR, 0, 0, 1 });
		immediateCmd->RHITextureBarrierCommand(
			{ m_texture, RHIResourceState::TransferDst, RHIResourceState::ShaderResource,
			  { TEXTURE_ASPECT_COLOR, 0, 1, 0, 1 } });
		immediateCmd->RHISubmit();
	}
	
	void Texture::LoadFromFile()
	{
		if (m_type == TextureType::TypeCube && m_paths.size() != 6)
		{
			SHZK_LOG_ERROR("File paths num incorrect for texture type CUBE");
			return;
		}

		m_name = std::filesystem::path(m_paths[0]).filename().generic_string();	// TODO: proper naming
		auto immediateCmd = RHI::Get()->GetCommandContextImmediate();
		for (uint32_t i = 0; i < m_paths.size(); ++i)
		{
			std::string& path = m_paths[i];
			int width, height, channels;
			stbi_uc* pixels = stbi_load(path.c_str(), &width, &height, &channels, 4);
			if (!pixels)
			{
				SHZK_LOG_ERROR("Texture::LoadFromFile ¡ª failed to load: {}", path);
				return;
			}

			if (!m_rhiInitialized)	// for TextureType::Cube, RHITexture should only be created once
			{
				m_extent = { (uint32_t)width, (uint32_t)height, 1 };
				m_mipLevels = (uint32_t)(std::floor(std::log2(std::max(width, height)))) + 1;
				InitRHI();

				m_rhiInitialized = true;
			}

			// upload
			uint32_t targetChannelCount = RHIUtil::FormatToChannelCount(m_format);
			uint32_t bufferSize = width * height * sizeof(uint8_t) * targetChannelCount;
			RHIBufferInfo bufferInfo = {
				.size = bufferSize,
				.memoryUsage = MemoryUsage::CPUOnly,
				.type = RESOURCE_TYPE_BUFFER,
				.creationFlag = BUFFER_CREATION_PERSISTENT_MAP
			};

			std::shared_ptr<RHIBuffer> stagingBuffer = RHI::Get()->CreateBuffer(bufferInfo);
			memcpy(stagingBuffer->Map(), pixels, bufferSize);

			immediateCmd->RHITextureBarrierCommand(
				{ m_texture,
				RHIResourceState::Undefined, RHIResourceState::TransferDst,
				{TEXTURE_ASPECT_COLOR, 0, m_mipLevels, i, 1} });
			immediateCmd->RHICopyBufferToTexture(stagingBuffer, 0, m_texture, {TEXTURE_ASPECT_COLOR, 0, i, 1});
			immediateCmd->RHISubmit();

			stbi_image_free(pixels);
		}

		immediateCmd->RHITextureBarrierCommand({m_texture,
				RHIResourceState::TransferDst, RHIResourceState::TransferSrc,
						{TEXTURE_ASPECT_COLOR, 0, m_mipLevels, 0, m_arrayLayer} });
		immediateCmd->RHIGenerateMips(m_texture);
		immediateCmd->RHITextureBarrierCommand({ m_texture,
			RHIResourceState::TransferSrc, RHIResourceState::ShaderResource,
					{TEXTURE_ASPECT_COLOR, 0, m_mipLevels, 0, m_arrayLayer} });
		immediateCmd->RHISubmit();
	}

	void Texture::LoadFromFileHDR()
	{
		int width, height, channels;
		float* pixels = stbi_loadf(m_paths[0].c_str(), &width, &height, &channels, 4);
		if (!pixels)
		{
			SHZK_LOG_ERROR("Texture::LoadFromFileHDR failed to load: {}", m_paths[0]);
			return;
		}

		m_name = std::filesystem::path(m_paths[0]).filename().generic_string();
		m_extent = { (uint32_t)width, (uint32_t)height, 1 };
		m_mipLevels = 1;
		m_arrayLayer = 1;

		InitRHI();

		const uint32_t texelCount = (uint32_t)width * height;
		const uint32_t bufferSize = texelCount * 4 * sizeof(uint16_t);
		std::vector<uint16_t> halfPixels(texelCount * 4);
		for (uint32_t i = 0; i < texelCount * 4; ++i)
			halfPixels[i] = Float32ToFloat16(pixels[i]);

		RHIBufferInfo bufferInfo = {
			.size = bufferSize,
			.memoryUsage = MemoryUsage::CPUOnly,
			.type = RESOURCE_TYPE_BUFFER,
			.creationFlag = BUFFER_CREATION_PERSISTENT_MAP
		};
		std::shared_ptr<RHIBuffer> stagingBuffer = RHI::Get()->CreateBuffer(bufferInfo);
		memcpy(stagingBuffer->Map(), halfPixels.data(), bufferSize);

		auto immediateCmd = RHI::Get()->GetCommandContextImmediate();
		immediateCmd->RHITextureBarrierCommand(
			{ m_texture, RHIResourceState::Undefined, RHIResourceState::TransferDst,
			  { TEXTURE_ASPECT_COLOR, 0, m_mipLevels, 0, 1 } });
		immediateCmd->RHICopyBufferToTexture(stagingBuffer, 0, m_texture, { TEXTURE_ASPECT_COLOR, 0, 0, 1 });
		immediateCmd->RHITextureBarrierCommand(
			{ m_texture, RHIResourceState::TransferDst, RHIResourceState::ShaderResource,
			  { TEXTURE_ASPECT_COLOR, 0, m_mipLevels, 0, 1 } });
		immediateCmd->RHISubmit();

		stbi_image_free(pixels);
	}

	void shzk::Texture::InitRHI()
	{
		ResourceType resourceType = (m_type == TextureType::TypeCube) ? (RESOURCE_TYPE_TEXTURE_CUBE | RESOURCE_TYPE_TEXTURE) : RESOURCE_TYPE_TEXTURE;
		if (RHIUtil::IsRWFormat(m_format))      resourceType |= RESOURCE_TYPE_RW_TEXTURE | RESOURCE_TYPE_RENDER_TARGET;		// TODO

		TextureAspectFlags aspects = RHIUtil::IsDepthStencilFormat(m_format) ? TEXTURE_ASPECT_DEPTH_STENCIL :
			RHIUtil::IsDepthFormat(m_format) ? TEXTURE_ASPECT_DEPTH :
			RHIUtil::IsStencilFormat(m_format) ? TEXTURE_ASPECT_STENCIL : TEXTURE_ASPECT_COLOR;

		bool force2D = m_extent.width == 1 && m_extent.height == 1;

		RHITextureInfo textureInfo = {
			.format = m_format,
			.extent = m_extent,
			.arrayLayers = m_arrayLayer,
			.mipLevels = m_mipLevels,
			.memoryUsage = MemoryUsage::GPUOnly,
			.type = resourceType,
			.creationFlag = force2D ? TEXTURE_CREATION_FORCE_2D : TEXTURE_CREATION_NONE };
		m_texture = RHI::Get()->CreateTexture(textureInfo);

		RHITextureViewInfo textureViewInfo = {
			.texture = m_texture,
			.format = m_format,
			.viewType = TextureTypeToTextureViewType(m_type),
			.subresourceRange = { aspects, 0, m_mipLevels, 0, m_arrayLayer} };
		m_textureView = RHI::Get()->CreateTextureView(textureViewInfo);

	}
}