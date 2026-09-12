#pragma once

#include <cstdint>
#include <glm/glm.hpp>

namespace shzk
{
	// enums
	enum DescriptorSetIndex : uint32_t
	{
		DESCRIPTORSET_INDEX_PER_FRAME = 0,
		DESCRIPTORSET_INDEX_MATERIAL = 1,

		DESCRIPTORSET_INDEX_MAX, 
	};

	enum PerFrameBinding : uint32_t
	{
		PER_FRAME_BINDING_VIEW				= 0,
		PER_FRAME_BINDING_IBL_IRRADIANCE	= 1,
		PER_FRAME_BINDING_IBL_SPECULAR		= 2,
		PER_FRAME_BINDING_IBL_BRDF_LUT		= 3,

		PER_FRAME_BINDING_MAX,
	};

	enum MaterialBinding : uint32_t
	{
		MATERIAL_BINDING_UNIFORM		= 0,   // uniform buffer: Material Data
		MATERIAL_BINDING_BASECOLOR		= 1,
		MATERIAL_BINDING_ARM			= 2,
		MATERIAL_BINDING_NORMAL			= 3,
		MATERIAL_BINDING_OCCLUSION		= 4,
		MATERIAL_BINDING_EMISSIVE		= 5,
		MATERIAL_BINDING_TEXTURE2D		= 6,   // [6, 10)  m_texture2D[4]
		MATERIAL_BINDING_TEXTURECUBE	= 10,  // [10, 14) m_textureCube[4]
		MATERIAL_BINDING_TEXTURE3D		= 14,  // [14, 18) m_texture3D[4]
	};

	// structs
	typedef struct PerFrameUniformShaderParameters
	{
		glm::mat4x4 view;
		glm::mat4x4 proj;
		glm::mat4x4 viewProj;

	} PerFrameUniformShaderParameters;

	typedef struct MaterialUniformShaderParameters
	{
		glm::vec4 baseColor;
		glm::vec3 emission;

		char  _padding[4];

		float metallic;
		float roughness;
		float alphaCutoff;
		uint32_t bUseVertexColor;

		// General slots
		std::array<int32_t, 8> ints{};    // 0: alpha mode 1£ºdouble sided 2: unlit
		std::array<float, 8>   floats{};  // 0: normal scale 1: occlusion strength
		std::array<glm::vec4, 8> colors{};
	} MaterialUniformShaderParameters;

}