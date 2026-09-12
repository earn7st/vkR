#pragma once

#include "runtime/rhi/RHIDefinitions.h"

namespace shzk
{
	static const RHIFormat HDR_COLOR_FORMAT = FORMAT_R16G16B16A16_SFLOAT;
	static const RHIFormat COLOR_FORMAT = FORMAT_R8G8B8A8_UNORM;
	static const RHIFormat DEPTH_FORMAT = FORMAT_D32_SFLOAT;

	static const glm::vec4 CLEAR_COLOR = { 0.1f, 0.2f, 0.4f, 1.0f };

	static const uint32_t IBL_IRR_SIZE = 32, IBL_SPEC_SIZE = 128, IBL_SPEC_MIPS = 5;
}