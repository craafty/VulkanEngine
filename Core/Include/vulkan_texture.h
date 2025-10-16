#pragma once

#include <vulkan/vulkan.h>

#include "vulkan_util.h"

namespace Engine {

	void CreateTextureSampler(VkDevice Device, VkSampler* pSampler, VkFilter MinFilter, VkFilter MaxFilter, VkSamplerAddressMode AddressMode);

}