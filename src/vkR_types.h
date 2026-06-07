// vulkan_guide.h : Include file for standard system include files,
// or project specific include files.
#pragma once

#include <memory>
#include <optional>
#include <string>
#include <vector>
#include <span>
#include <array>
#include <functional>
#include <deque>

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>

#include <vma/vk_mem_alloc.h>

#include <fmt/core.h>
#include <fmt/format.h>

#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>


#define VK_CHECK(x)                                                     \
    do {                                                                \
        VkResult err = x;                                               \
        if (err) {                                                      \
            fmt::println("[Vulkan Err]", string_VkResult(err)); \
            abort();                                                    \
        }                                                               \
    } while (0)