#pragma once
#include <memory>
#include <string>
#include <vector>
#include <array>
#include <glm/glm.hpp>

namespace shzk
{
#if WIN32
	#define MAX_QUEUE_CNT 2
#else
	#define MAX_QUEUE_CNT 1
#endif 

#define FRAMES_IN_FLIGHT 2
#define RESOURCE_DEFERRED_DELETE_FRAMES 4
#define MAX_RENDER_TARGETS 8
	/** The maximum number of vertex elements which can be used by a vertex declaration. */
#define MAX_VERTEX_ELEMENT_COUNT 16

	class RHIQueue;
	class RHISurface;
	class RHISwapchain;
	class RHIBuffer;
	class RHITexture;
	class RHITextureView;
	class RHIShader;
	class RHISampler;
	class RHIGraphicsPipeline;
	class RHIComputePipeline;
	class RHIDescriptorSet;
	class RHIRootSignature;

// enum classes
	enum class RHIResourceType : uint32_t
	{
		Buffer,
		Texture,
		TextureView,
		Sampler,
		Shader,

		RootSignature,
		DescriptorSet,

		GraphicsPipeline,
		ComputePipeline,

		Max,
	};

	enum class RHIBackendType : uint32_t
	{
		Vulkan,
		Max = 0x7FFFFFFF
	};

	enum class RHIQueueType : uint32_t
	{
		Graphics,
		Compute,
		Transfer,
		Max
	};

	enum class RHIResourceState : uint32_t
	{
		Undefined,
		Common,
		Present,
		ColorAttachment,
		TransferSrc,
		TransferDst,
		VertexBuffer,
		IndexBuffer,
		DepthStencilAttachment,
		UnorderedAccess,
		ShaderResource,
		IndirectArgument,		// not impl
		AccelerationStructure,	// not impl

		Max,
	};

	enum class MemoryUsage : uint32_t
	{
		Unknown,
		GPUOnly,		// 仅GPU使用，在VRAM显存上分配，不可绑定
		CPUOnly,		// HOST_VISIBLE &&  HOST_COHERENT 及时同步，不需要flush到GPU，GPU可访问但是很慢
		CPUToGPU,		// HOST_VISIBLE CPU端uncached，用于CPU端频繁进行数据写入，GPU端对数据进行读取
		GPUToCPU,		// HOST_VISIBLE CPU端cached，用于被GPU写入且被CPU读取

		Max = 0x7FFFFFFF,
	};

// enums
	enum RHIFormat : uint32_t
	{
		FORMAT_UKNOWN = 0,

		FORMAT_R8_SRGB,
		FORMAT_R8G8_SRGB,
		FORMAT_R8G8B8_SRGB,
		FORMAT_R8G8B8A8_SRGB,
		FORMAT_B8G8R8A8_SRGB,

		FORMAT_R16_SFLOAT,
		FORMAT_R16G16_SFLOAT,
		FORMAT_R16G16B16_SFLOAT,
		FORMAT_R16G16B16A16_SFLOAT,
		FORMAT_R32_SFLOAT,
		FORMAT_R32G32_SFLOAT,
		FORMAT_R32G32B32_SFLOAT,
		FORMAT_R32G32B32A32_SFLOAT,

		FORMAT_R8_UNORM,
		FORMAT_R8G8_UNORM,
		FORMAT_R8G8B8_UNORM,
		FORMAT_R8G8B8A8_UNORM,
		FORMAT_B8G8R8A8_UNORM,
		FORMAT_R16_UNORM,
		FORMAT_R16G16_UNORM,
		FORMAT_R16G16B16_UNORM,
		FORMAT_R16G16B16A16_UNORM,

		FORMAT_R8_SNORM,
		FORMAT_R8G8_SNORM,
		FORMAT_R8G8B8_SNORM,
		FORMAT_R8G8B8A8_SNORM,
		FORMAT_R16_SNORM,
		FORMAT_R16G16_SNORM,
		FORMAT_R16G16B16_SNORM,
		FORMAT_R16G16B16A16_SNORM,

		FORMAT_R8_UINT,
		FORMAT_R8G8_UINT,
		FORMAT_R8G8B8_UINT,
		FORMAT_R8G8B8A8_UINT,
		FORMAT_R16_UINT,
		FORMAT_R16G16_UINT,
		FORMAT_R16G16B16_UINT,
		FORMAT_R16G16B16A16_UINT,
		FORMAT_R32_UINT,
		FORMAT_R32G32_UINT,
		FORMAT_R32G32B32_UINT,
		FORMAT_R32G32B32A32_UINT,

		FORMAT_R8_SINT,
		FORMAT_R8G8_SINT,
		FORMAT_R8G8B8_SINT,
		FORMAT_R8G8B8A8_SINT,
		FORMAT_R16_SINT,
		FORMAT_R16G16_SINT,
		FORMAT_R16G16B16_SINT,
		FORMAT_R16G16B16A16_SINT,
		FORMAT_R32_SINT,
		FORMAT_R32G32_SINT,
		FORMAT_R32G32B32_SINT,
		FORMAT_R32G32B32A32_SINT,

		FORMAT_D32_SFLOAT,
		FORMAT_D32_SFLOAT_S8_UINT,
		FORMAT_D24_UNORM_S8_UINT,

		FORMAT_A2R10G10B10_SNORM,
		FORMAT_A2R10G10B10_UNORM,
		FORMAT_A2R10G10B10_SINT,
		FORMAT_A2R10G10B10_UINT,
		FORMAT_B10G11R11_UFLOAT,
		FORMAT_E5B9G9R9_UFLOAT,

		FORMAT_MAX_ENUM,
	};

	enum ResourceTypeBits : uint32_t
	{
		RESOURCE_TYPE_NONE						= 0,
		RESOURCE_TYPE_SAMPLER					= 1 << 0,
		RESOURCE_TYPE_TEXTURE					= 1 << 1,
		RESOURCE_TYPE_RW_TEXTURE				= 1 << 2,
		RESOURCE_TYPE_TEXTURE_CUBE				= 1 << 3,
		RESOURCE_TYPE_RENDER_TARGET				= 1 << 4,
		RESOURCE_TYPE_COMBINED_IMAGE_SAMPLER	= 1 << 5,
		RESOURCE_TYPE_BUFFER					= 1 << 6,
		RESOURCE_TYPE_RW_BUFFER					= 1 << 7,
		RESOURCE_TYPE_UNIFORM_BUFFER			= 1 << 8,
		RESOURCE_TYPE_VERTEX_BUFFER				= 1 << 9,
		RESOURCE_TYPE_INDEX_BUFFER				= 1 << 10,
		RESOURCE_TYPE_INDIRECT_BUFFER			= 1 << 11,
		RESOURCE_TYPE_TEXEL_BUFFER				= 1 << 12,
		RESOURCE_TYPE_RW_TEXEL_BUFFER			= 1 << 13,
		// RESOURCE_TYPE_RAY_TRACING				= 1 << 14,

		RESOURCE_TYPE_MAX_ENUM = 0x7FFFFFFF,
	};
	typedef uint32_t ResourceType;

	enum BufferCreationFlagBits : uint32_t
	{
		BUFFER_CREATION_NONE					= 0,
		BUFFER_CREATION_PERSISTENT_MAP			= 1 << 0,
		BUFFER_CREATION_FORCE_ALIGNMENT			= 1 << 1,

		BUFFER_CREATION_MAX_ENUM = 0x7FFFFFFF,
	};
	typedef uint32_t BufferCreationFlags;

	enum TextureCreationFlagBits : uint32_t
	{
		TEXTURE_CREATION_NONE		= 0,
		TEXTURE_CREATION_FORCE_2D	= 1 << 0,
		TEXTURE_CREATION_FORCE_3D	= 1 << 1,

		TEXTURE_CREATION_MAX_ENUM = 0x7FFFFFFF,
	};
	typedef uint32_t TextureCreationFlags;

	enum TextureAspectFlagBits : uint32_t
	{
		TEXTURE_ASPECT_NONE		= 0,
		TEXTURE_ASPECT_COLOR	= 1 << 0,
		TEXTURE_ASPECT_DEPTH	= 1 << 1,
		TEXTURE_ASPECT_STENCIL	= 1 << 2,

		TEXTURE_ASPECT_DEPTH_STENCIL = TEXTURE_ASPECT_DEPTH | TEXTURE_ASPECT_STENCIL,

		TEXTURE_ASPECT_MAX_ENUM = 0x7FFFFFFF,
	};
	typedef uint32_t TextureAspectFlags;

	enum class TextureViewType : uint32_t
	{
		View1D,
		View2D,
		View3D,
		ViewCube,
		View1DArray,
		View2DArray,
		ViewCubeArray,

		Max,
	};

	enum ShaderFrequencyBits : uint32_t
	{
		SHADER_FREQUENCY_COMPUTE		= 1 << 0,
		SHADER_FREQUENCY_VERTEX			= 1 << 1,
		SHADER_FREQUENCY_FRAGMENT		= 1 << 2,
		SHADER_FREQUENCY_GEOMETRY		= 1 << 3,
		SHADER_FREQUENCY_RAY_GEN		= 1 << 4,
		SHADER_FREQUENCY_CLOSEST_HIT	= 1 << 5,
		SHADER_FREQUENCY_RAY_MISS		= 1 << 6,
		SHADER_FREQUENCY_INTERSECTION	= 1 << 7,
		SHADER_FREQUENCY_ANY_HIT		= 1 << 8,
		SHADER_FREQUENCY_MESH			= 1 << 9,

		SHADER_FREQUENCY_GRAPHICS = SHADER_FREQUENCY_VERTEX |
		SHADER_FREQUENCY_FRAGMENT |
		SHADER_FREQUENCY_GEOMETRY |
		SHADER_FREQUENCY_MESH,
		SHADER_FREQUENCY_RAY_TRACING = SHADER_FREQUENCY_RAY_GEN |
		SHADER_FREQUENCY_CLOSEST_HIT |
		SHADER_FREQUENCY_RAY_MISS |
		SHADER_FREQUENCY_INTERSECTION |
		SHADER_FREQUENCY_ANY_HIT,
		SHADER_FREQUENCY_ALL = SHADER_FREQUENCY_GRAPHICS |
		SHADER_FREQUENCY_COMPUTE |
		SHADER_FREQUENCY_RAY_TRACING,

		SHADER_FREQUENCY_MAX_ENUM = 0x7FFFFFFF,
	};
	typedef uint32_t ShaderFrequency;

	enum class FilterType : uint8_t
	{
		Nearest,
		Linear,

		Max,
	};

	enum class CompareFunction : uint8_t
	{
		Less,
		LessEqual,
		Greater,
		GreaterEqual,
		Equal,
		NotEqual,
		Never,
		Always,

		Max,

		// Utility enumerations
		DepthNearOrEqual = GreaterEqual,
		DepthNear = Greater,
		DepthFartherOrEqual = LessEqual,
		DepthFarther = Less,
	};

	enum class StencilOp : uint8_t
	{
		Keep,
		Zero,
		Replace,
		SaturatedIncrement,
		SaturatedDecrement,
		Invert,
		Increment,
		Decrement,
		
		Max,
	};

	enum class BlendOp : uint8_t
	{
		Add,
		Subtract, 
		Min,
		Max,
		ReverseSubtract,

		MaxEnum,
	};

	enum class BlendFactor : uint8_t
	{
		Zero,
		One,
		SourceColor,
		InverseSourceColor,
		SourceAlpha,
		InverseSourceAlpha,
		DestAlpha,
		InverseDestAlpha,
		DestColor,
		InverseDestColor,
		ConstantBlendFactor,
		InverseConstantBlendFactor,
		Source1Color,
		InverseSource1Color,
		Source1Alpha,
		InverseSource1Alpha,
		
		Max,
	};

	enum ColorWriteMask
	{
		COLOR_WRITE_MASK_RED = 0x01,
		COLOR_WRITE_MASK_GREEN = 0x02,
		COLOR_WRITE_MASK_BLUE = 0x04,
		COLOR_WRITE_MASK_ALPHA = 0x08,

		COLOR_WRITE_MASK_NONE = 0,
		COLOR_WRITE_MASK_RGB = COLOR_WRITE_MASK_RED | COLOR_WRITE_MASK_GREEN | COLOR_WRITE_MASK_BLUE,
		COLOR_WRITE_MASK_RGBA = COLOR_WRITE_MASK_RED | COLOR_WRITE_MASK_GREEN | COLOR_WRITE_MASK_BLUE | COLOR_WRITE_MASK_ALPHA,
		COLOR_WRITE_MASK_RG = COLOR_WRITE_MASK_RED | COLOR_WRITE_MASK_GREEN,
		COLOR_WRITE_MASK_BA = COLOR_WRITE_MASK_BLUE | COLOR_WRITE_MASK_ALPHA,

		Max,
	};

	enum class RasterizerFillMode : uint8_t
	{
		Point		= 0,
		Wireframe	= 1,
		Solid		= 2,

		Max,
	};

	enum class RasterizerCullMode : uint8_t
	{
		None		= 0,
		CW			= 1,
		CCW			= 2,

		Max,
	};

	enum class RasterizerDepthClipMode : uint8_t
	{
		DepthClip	= 0,
		DepthClamp	= 1,

		Max,
	};

	enum class PrimitiveType : uint8_t
	{
		TriangleList	= 0,
		TriangleStrip	= 1,
		LineList		= 2,
		LineStrip		= 3,
		PointList		= 4,

		Max,
	};

	enum class VertexElementType : uint8_t
	{
		None,
		Float1,
		Float2,
		Float3,
		Float4,
		PackedNormal,	// FPackedNormal
		UByte4,
		UByte4N,
		Color,
		Short2,
		Short4,
		Short2N,		// 16 bit word normalized to (value/32767.0,value/32767.0,0,0,1)
		Half2,			// 16 bit float using 1 bit sign, 5 bit exponent, 10 bit mantissa 
		Half4,
		Short4N,		// 4 X 16 bit word, normalized 
		UShort2,
		UShort4,
		UShort2N,		// 16 bit word normalized to (value/65535.0,value/65535.0,0,0,1)
		UShort4N,		// 4 X 16 bit word unsigned, normalized 
		URGB10A2N,		// 10 bit r, g, b and 2 bit a normalized to (value/1023.0f, value/1023.0f, value/1023.0f, value/3.0f)
		UInt,

		Max,
	};

	enum class AttachmentLoadOp : uint8_t
	{
		Load = 0,
		Clear = 1,
		DontCare = 2,

		Max,
	};

	enum class AttachmentStoreOp : uint8_t
	{
		Store = 0,
		DontCare = 1,

		Max,
	};

	enum class SamplerMipmapMode : uint8_t
	{
		Nearest = 0,
		Linear = 1,

		Max,
	};

	enum class SamplerAddressMode : uint8_t
	{
		Repeat = 0,
		MirroredRepeat = 1,
		ClampToEdge = 2,
		ClampToBorder = 3,
		MirrorClampToEdge = 4,
	
		Max,
	};

// structs
	typedef struct Extent2D
	{
		uint32_t width	= 0;
		uint32_t height = 0;
	} Extent2D;

	typedef struct Extent3D
	{
		uint32_t width	= 0;
		uint32_t height = 0;
		uint32_t depth	= 0;
	} Extent3D;

	typedef struct Offset2D
	{
		uint32_t x = 0;
		uint32_t y = 0;

		friend Offset2D operator+(const Offset2D& a, const Offset2D& b)
		{
			return { .x = a.x + b.x, .y = a.y + b.y };
		}

		friend Offset2D operator-(const Offset2D& a, const Offset2D& b)
		{
			return { .x = a.x - b.x, .y = a.y - b.y };
		}
	} Offset2D;

	typedef struct Offset3D
	{
		uint32_t x = 0;
		uint32_t y = 0;
		uint32_t z = 0;

		friend Offset3D operator+(const Offset3D& a, const Offset3D& b)
		{
			return { .x = a.x + b.x, .y = a.y + b.y, .z = a.z + b.z };
		}

		friend Offset3D operator-(const Offset3D& a, const Offset3D& b)
		{
			return { .x = a.x - b.x, .y = a.y - b.y, .z = a.z - b.z };
		}
	} Offset3D;

	typedef struct Rect2D
	{
		Offset2D    offset = {};
		Extent2D    extent = {};
	} Rect2D;

	typedef struct TextureSubresourceRange
	{
		TextureAspectFlags	  aspect			= TEXTURE_ASPECT_NONE;
		uint32_t              baseMipLevel		= 0;
		uint32_t              levelCount		= 0;
		uint32_t              baseArrayLayer	= 0;
		uint32_t              layerCount		= 0;

		uint32_t			  __padding			= 0;

		friend bool operator==(const TextureSubresourceRange& a, const TextureSubresourceRange& b)
		{
			return 	a.aspect == b.aspect &&
				a.baseMipLevel == b.baseMipLevel &&
				a.levelCount == b.levelCount &&
				a.baseArrayLayer == b.baseArrayLayer &&
				a.layerCount == b.layerCount;
		}

		bool IsDefault()
		{
			return 	aspect == TEXTURE_ASPECT_NONE &&
				baseMipLevel == 0 &&
				levelCount == 0 &&
				baseArrayLayer == 0 &&
				layerCount == 0;
		}

	} TextureSubresourceRange;

	typedef struct TextureSubresourceLayers
	{
		TextureAspectFlags	  aspect			= TEXTURE_ASPECT_NONE;
		uint32_t              mipLevel			= 0;
		uint32_t              baseArrayLayer	= 0;
		uint32_t              layerCount		= 0;

		friend bool operator==(const TextureSubresourceLayers& a, const TextureSubresourceLayers& b)
		{
			return 	a.aspect == b.aspect &&
				a.mipLevel == b.mipLevel &&
				a.baseArrayLayer == b.baseArrayLayer &&
				a.layerCount == b.layerCount;
		}

		bool IsDefault()
		{
			return 	aspect == TEXTURE_ASPECT_NONE &&
				mipLevel == 0 &&
				baseArrayLayer == 0 &&
				layerCount == 0;
		}

	} TextureSubresourceLayers;

	typedef struct ShaderResourceEntry
	{
		uint32_t set = 0;
		uint32_t binding = 0;
		uint32_t size = 1;
		ShaderFrequency frequency = SHADER_FREQUENCY_ALL;

		ResourceType type = RESOURCE_TYPE_NONE;

		friend bool operator== (const ShaderResourceEntry& a, const ShaderResourceEntry& b)
		{
			return  a.set == b.set &&
				a.binding == b.binding &&
				a.size == b.size &&
				a.frequency == b.frequency &&
				a.type == b.type;
		}
	} ShaderResourceEntry;

	typedef struct PushConstantInfo
	{
		uint32_t offset = 0;
		uint32_t size = 128;
		ShaderFrequency frequency = SHADER_FREQUENCY_ALL;
		friend bool operator== (const PushConstantInfo& a, const PushConstantInfo& b)
		{
			return  a.offset == b.offset &&
				a.size == b.size &&
				a.frequency == b.frequency;
		}
	} PushConstantInfo;

	typedef struct ClearAttachment
	{
		uint32_t binding = 0;
		TextureAspectFlags aspect = TEXTURE_ASPECT_NONE;
		glm::vec4 clearColor = {};
	} ClearAttachment;

// RHI
	typedef struct RHIInfo
	{
		RHIBackendType type = RHIBackendType::Vulkan;
		bool debug = true;
	} RHIInfo;

	typedef struct RHIQueueInfo
	{
		RHIQueueType type;
		uint32_t index;
	} RHIQueueInfo;

	typedef struct RHISwapchainInfo
	{
		std::shared_ptr<RHISurface> surface;
		std::shared_ptr<RHIQueue> presentQueue;

		uint32_t imageCount;
		Extent2D extent;
		RHIFormat format;
	} RHISwapchainInfo;

	typedef struct RHICommandPoolInfo
	{
		std::shared_ptr<RHIQueue> queue;
	} RHICommandPoolInfo;

	typedef struct RHIBufferInfo
	{
		uint64_t size;

		MemoryUsage memoryUsage = MemoryUsage::GPUOnly;
		ResourceType type = RESOURCE_TYPE_BUFFER;

		BufferCreationFlags creationFlag = BUFFER_CREATION_NONE;

	} RHIBufferInfo;

	typedef struct RHITextureInfo
	{
		RHIFormat format;
		Extent3D extent;
		uint32_t arrayLayers = 1;
		uint32_t mipLevels = 1;

		MemoryUsage memoryUsage = MemoryUsage::GPUOnly;
		ResourceType type		= RESOURCE_TYPE_TEXTURE;

		TextureCreationFlags creationFlag = TEXTURE_CREATION_NONE;

		// for hash
		/*
		friend bool operator== (const RHITextureInfo& a, const RHITextureInfo& b)
		{
			return  a.format == b.format &&
				a.extent == b.extent &&
				a.arrayLayers == b.arrayLayers &&
				a.mipLevels == b.mipLevels &&
				a.memoryUsage == b.memoryUsage &&
				a.type == b.type &&
				a.creationFlag == b.creationFlag;
		}*/
	} RHITextureInfo;

	typedef struct RHITextureViewInfo
	{
		std::shared_ptr<RHITexture> texture;
		RHIFormat format = FORMAT_UKNOWN;
		TextureViewType viewType = TextureViewType::View2D;

		TextureSubresourceRange subresourceRange{}; // default
	} RHITextureViewInfo;
	
	typedef struct RHIBufferBarrier
	{
		std::shared_ptr<RHIBuffer> buffer;
		RHIResourceState srcState;
		RHIResourceState dstState;

		uint32_t offset = 0;
		uint32_t size = 0;

	} RHIBufferBarrier;
	
	typedef struct RHITextureBarrier
	{
		std::shared_ptr<RHITexture> texture;
		RHIResourceState srcState;
		RHIResourceState dstState;

		TextureSubresourceRange subresource = {};	// default of RHITexture

	} RHITextureBarrier;

	typedef struct RHISamplerInfo
	{
		FilterType magFilter = FilterType::Linear;
		FilterType minFilter = FilterType::Linear;
		SamplerMipmapMode mipmapMode = SamplerMipmapMode::Linear;
		SamplerAddressMode addressModeU = SamplerAddressMode::Repeat;
		SamplerAddressMode addressModeV = SamplerAddressMode::Repeat;
		SamplerAddressMode addressModeW = SamplerAddressMode::Repeat;

		float mipLodBias = 0.0f;
		bool bAnisotropyEnable = false;
		float maxAnisotropy = 0.0f;

		bool bCompareEnable = false;
		CompareFunction compareFunction = CompareFunction::Never;

		float minLod = 0.f;
		float maxLod = 1000.f;
		//SamplerReductionMode reductionMode = SAMPLER_REDUCTION_MODE_WEIGHTED_AVERAGE;
	} RHISamplerInfo;

	typedef struct RHIShaderInfo
	{
		std::string entry = "main";

		ShaderFrequency frequency;
		std::vector<uint8_t> code;
	} RHIShaderInfo;

	typedef struct RHIDescriptorUpdateInfo
	{
		uint32_t binding = 0;
		uint32_t index = 0;

		ResourceType resourceType = RESOURCE_TYPE_NONE;

		std::shared_ptr<RHIBuffer> buffer;
		std::shared_ptr<RHITextureView> textureView;
		std::shared_ptr<RHISampler> sampler;

		uint64_t bufferOffset = 0;
		uint64_t bufferRange = 0;
	} RHIDescriptorUpdateInfo;

	typedef struct VertexElement
	{
		uint8_t streamIndex = 0;
		uint8_t offset = 0;
		VertexElementType type = VertexElementType::None;
		uint8_t attributeIndex = 0;
		uint16_t stride = 0;

		bool bUseInstanceIndex = false;	// currently only support per vertex input rate

		bool IsEmpty() const 
		{
			return streamIndex == 0 &&
				offset == 0 &&
				type == VertexElementType::None &&
				attributeIndex == 0 &&
				stride == 0;
		}

		friend bool operator== (const VertexElement& a, const VertexElement& b)
		{
			return  a.streamIndex == b.streamIndex &&
				a.offset == b.offset &&
				a.type == b.type &&
				a.stride == b.stride &&
				a.attributeIndex == b.attributeIndex;
		}
	} VertexElement;

	typedef struct RHIVertexDeclaration
	{
		std::array<VertexElement, MAX_VERTEX_ELEMENT_COUNT> elements;

		friend bool operator==(const RHIVertexDeclaration& a, const RHIVertexDeclaration& b)
		{
			if (a.elements.size() != b.elements.size()) return false;
			return a.elements == b.elements;
		}
	} RHIVertexDeclaration;

	typedef struct BoundShaderStateInput
	{
		std::shared_ptr<RHIVertexDeclaration> declaration;
		std::shared_ptr<RHIShader> vertexShader;
		std::shared_ptr<RHIShader> fragmentShader;

		friend bool operator==(const BoundShaderStateInput& a, const BoundShaderStateInput& b)
		{
			return a.declaration == b.declaration &&
				a.vertexShader == b.vertexShader &&
				a.fragmentShader == b.fragmentShader;
		}

	} BoundShaderStateInput;

	typedef struct RHIBlendState
	{
		struct RenderTarget
		{
			BlendOp colorBlendOp;
			BlendFactor colorSrcBlend;
			BlendFactor colorDstBlend;

			BlendOp alphaBlendOp;
			BlendFactor alphaSrcBlend;
			BlendFactor alphaDstBlend;

			ColorWriteMask colorWriteMask;

			bool bEnable;

			friend bool operator==(const RenderTarget& a, const RenderTarget& b)
			{
				return  a.colorBlendOp == b.colorBlendOp &&
					a.colorSrcBlend == b.colorSrcBlend &&
					a.colorDstBlend == b.colorDstBlend &&
					a.alphaBlendOp == b.alphaBlendOp &&
					a.alphaSrcBlend == b.alphaSrcBlend &&
					a.alphaDstBlend == b.alphaDstBlend &&
					a.colorWriteMask == b.colorWriteMask &&
					a.bEnable == b.bEnable;
			}
		};

		std::array<RenderTarget, MAX_RENDER_TARGETS> renderTargets;

		friend bool operator==(const RHIBlendState& a, const RHIBlendState& b)
		{
			return	a.renderTargets == b.renderTargets;
		}

	} RHIBlendState;

	typedef struct RHIRasterizerState
	{
		RasterizerFillMode fillMode = RasterizerFillMode::Solid;
		RasterizerCullMode cullMode = RasterizerCullMode::CW;

		float depthBias				= 0.0f;
		float slopeScaleDepthBias	= 0.0f;
		RasterizerDepthClipMode depthClipMode = RasterizerDepthClipMode::DepthClip;

		friend bool operator==(const RHIRasterizerState& a, const RHIRasterizerState& b)
		{
			return	a.fillMode == b.fillMode &&
				a.cullMode == b.cullMode &&
				a.depthBias == b.depthBias &&
				a.slopeScaleDepthBias == b.slopeScaleDepthBias &&
				a.depthClipMode == b.depthClipMode;
		}

	} RHIRasterizerState;

	typedef struct RHIDepthStencilState
	{
		bool bEnableDepthWrite		= true;
		bool bEnableDepthTest		= true;
		CompareFunction depthTest	= CompareFunction::LessEqual;

		friend bool operator==(const RHIDepthStencilState& a, const  RHIDepthStencilState& b)
		{
			return	a.bEnableDepthTest == b.bEnableDepthTest &&
				a.bEnableDepthWrite == b.bEnableDepthWrite &&
				a.depthTest == b.depthTest;
		}
	} RHIDepthStencilState;

	typedef struct RHIRenderPassAttachment
	{
		std::shared_ptr<RHITextureView> view;

		RHIResourceState layout = RHIResourceState::ColorAttachment;

		AttachmentLoadOp  loadOp = AttachmentLoadOp::Clear;
		AttachmentStoreOp storeOp = AttachmentStoreOp::Store;

		glm::vec4	clearColor = { 0.0f, 0.0f, 0.0f, 0.0f };
		float		clearDepth = 1.0f;
		uint32_t	clearStencil = 0;
	} RHIRenderPassAttachment;

// PipelineState, Pipeline
	typedef struct RHIRootSignatureInfo
	{
		RHIRootSignatureInfo& AddPushConstant(const PushConstantInfo& pushConstant) { pushConstants.push_back(pushConstant); return *this; }
		RHIRootSignatureInfo& AddEntry(const ShaderResourceEntry& entry);
		RHIRootSignatureInfo& AddEntry(const RHIRootSignatureInfo& other);
		
		// RHIRootSignatureInfo& AddEntryFromReflect(std::shared_ptr<RHIShader> shader);	// TODO: shader reflection
		const std::vector<PushConstantInfo>& GetPushConstants() const { return pushConstants; }
		const std::vector<ShaderResourceEntry>& GetEntries() const { return entries; }

	private:
		std::vector<ShaderResourceEntry> entries;
		std::vector<PushConstantInfo> pushConstants;

	} RHIRootSignatureInfo;

	// Since we use Dynamic Rendering, so no actual VkRenderPass or VkFramebuffer are created
	// equals to RenderPass + Framebuffer abstraction
	typedef struct RHIRenderPassInfo	
	{
		Extent2D renderArea;
		uint32_t layerCount = 1;
		uint32_t viewMask = 0;

		std::array<RHIRenderPassAttachment, MAX_RENDER_TARGETS> colorAttachments;
		RHIRenderPassAttachment	depthStencilAttachment;
	} RHIRenderPassInfo;

	typedef struct RHIGraphicsPipelineInfo
	{
		std::shared_ptr<RHIShader> vertexShader;
		std::shared_ptr<RHIShader> fragmentShader;
		
		std::shared_ptr<RHIRootSignature> rootSignature;	

		std::shared_ptr<RHIVertexDeclaration>	vertexInputState = {};
		PrimitiveType			primitiveType = PrimitiveType::TriangleList;
		RHIRasterizerState		rasterizerState = {};
		RHIBlendState			blendState = {};
		RHIDepthStencilState	depthStencilState = {};

		// Output (RenderPass)
		std::array<RHIFormat, MAX_RENDER_TARGETS>	colorAttachmentFormats = { FORMAT_UKNOWN };
		RHIFormat									depthStencilAttachmentFormat = FORMAT_UKNOWN;	
		uint32_t viewMask = 0b00000000;	// multiview: single view
		// uint32_t numSamples = 1;		// TODO: MSAA

		friend bool operator== (const RHIGraphicsPipelineInfo& a, const RHIGraphicsPipelineInfo& b)
		{
			return  a.vertexShader.get() == b.vertexShader.get() &&
				a.fragmentShader.get() == b.fragmentShader.get() &&
				a.rootSignature.get() == b.rootSignature.get() &&
				a.vertexInputState == b.vertexInputState &&
				a.primitiveType == b.primitiveType &&
				a.rasterizerState == b.rasterizerState &&
				a.blendState == b.blendState &&
				a.depthStencilState == b.depthStencilState &&
				a.colorAttachmentFormats == b.colorAttachmentFormats &&
				a.depthStencilAttachmentFormat == b.depthStencilAttachmentFormat &&
				a.viewMask == b.viewMask;
		}

	} RHIGraphicsPipelineInfo;

	typedef struct RHIComputePipelineInfo
	{
		std::shared_ptr<RHIShader> computeShader;
		std::shared_ptr<RHIRootSignature> rootSignature;
		friend bool operator== (const RHIComputePipelineInfo& a, const RHIComputePipelineInfo& b)
		{
			return  a.computeShader.get() == b.computeShader.get() &&
				a.rootSignature.get() == b.rootSignature.get();
		}
	} RHIComputePipelineInfo;

// Cast helper
// TODO: Type Traits - learnt but decided not to implement, because not all RHIthings are RHIResouce
	template<typename Concrete, typename Abstract>
	inline Concrete* CastTo(Abstract* ptr)
	{
		return static_cast<Concrete*>(ptr);
	}

	template<typename Concrete, typename Abstract>
	inline std::shared_ptr<Concrete> CastTo(const std::shared_ptr<Abstract>& ptr)
	{
		return std::static_pointer_cast<Concrete>(ptr);
	}
}