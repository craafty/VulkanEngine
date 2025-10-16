#pragma once

#include <vector>

#include <vulkan/vulkan.h>

namespace Engine {

	struct RangeDesc {
		VkDeviceSize m_offset = 0;
		VkDeviceSize m_range = 0;
	};

	struct SubmeshRanges {
		RangeDesc m_vbRange;
		RangeDesc m_ibRange;
		RangeDesc m_uniformRange;
	};

	struct TextureInfo {
		VkSampler m_sampler;
		VkImageView m_imageView;
	};

	struct ModelDesc {
		VkBuffer m_vb;
		VkBuffer m_ib;
		std::vector<VkBuffer> m_uniforms;
		std::vector<TextureInfo> m_materials;
		std::vector<SubmeshRanges> m_ranges;
	};

};