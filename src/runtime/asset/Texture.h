#pragma once

#include "Asset.h"
#include "runtime/rhi/RHIDefinitions.h"

#include <string>

namespace shzk
{
    enum class TextureType
    {
        Type2D,
        Type2DArray,
        TypeCube,
        Type3D,
        TypeEquirectangular,
        
        Max,
    };

    TextureViewType TextureTypeToTextureViewType(TextureType type);

    class Texture : public Asset
    {
    public:
        Texture() = delete;
		Texture(std::string path, TextureType type = TextureType::Type2D, RHIFormat format = RHIFormat::FORMAT_R8G8B8A8_SRGB);
		Texture(const std::vector<std::string>& paths, TextureType type = TextureType::TypeCube, RHIFormat format = RHIFormat::FORMAT_R8G8B8A8_SRGB);
        Texture(Extent2D extent, glm::vec4 rgba, RHIFormat format = RHIFormat::FORMAT_R8G8B8A8_UNORM);
        ~Texture() = default;

        inline TextureType GetType() const { return m_type; }
        inline const std::vector<std::string>& GetPaths() const { return m_paths; }
        inline Extent3D GetExtent3D() const { return m_extent; }
        inline uint32_t GetMipLevels() const { return m_mipLevels; }
        inline uint32_t GetArrayLayers() const { return m_arrayLayer; }
        inline RHIFormat GetFormat() const { return m_format; }
        inline std::shared_ptr<RHITexture> GetRHITexture() const { return m_texture; }
        inline std::shared_ptr<RHITextureView> GetRHITextureView() const { return m_textureView; }
        inline bool IsRHIInitialized() const { return m_rhiInitialized; }

    private:
        TextureType m_type = TextureType::Max;
        std::vector<std::string> m_paths;

        Extent3D    m_extent{};
        uint32_t    m_mipLevels = 1;
        uint32_t    m_arrayLayer = 1;
        RHIFormat   m_format = FORMAT_UKNOWN;
        std::shared_ptr<RHITexture> m_texture;
        std::shared_ptr<RHITextureView> m_textureView;
        bool m_rhiInitialized = false;

    private:
        void LoadFromFile();
        void LoadFromFileHDR();     // environment: .hdr, equirectangular map
        void InitRHI();
    };
}