#include <stdlib.h>

#include "vulkan_texture.h"

namespace Engine {



	void CreateTextureSampler(VkDevice Device, VkSampler* pSampler, VkFilter MinFilter, VkFilter MaxFilter, VkSamplerAddressMode AddressMode)
	{
		const VkSamplerCreateInfo SamplerInfo = {
			.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.magFilter = VK_FILTER_LINEAR,
			.minFilter = VK_FILTER_LINEAR,
			.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
			.addressModeU = AddressMode, // VK_SAMPLER_ADDRESS_MODE_REPEAT,
			.addressModeV = AddressMode, // VK_SAMPLER_ADDRESS_MODE_REPEAT,
			.addressModeW = AddressMode, // VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE VK_SAMPLER_ADDRESS_MODE_REPEAT,
			.mipLodBias = 0.0f,
			.anisotropyEnable = VK_FALSE,
			.maxAnisotropy = 1,
			.compareEnable = VK_FALSE,
			.compareOp = VK_COMPARE_OP_ALWAYS,
			.minLod = 0.0f,
			.maxLod = 0.0f,
			.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
			.unnormalizedCoordinates = VK_FALSE
		};

		VkResult res = vkCreateSampler(Device, &SamplerInfo, VK_NULL_HANDLE, pSampler);
		CHECK_VK_RESULT(res, "vkCreateSampler");
	}

}