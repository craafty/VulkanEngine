#pragma once

#include <string>

#include <vulkan/vulkan.h>


namespace Engine {

	class VulkanCore;

	class VulkanTexture {
	public:
		VulkanTexture() {}

		VulkanTexture(VulkanCore* pVulkanCore) { m_pVulkanCore = pVulkanCore; }

		VkImage m_image = VK_NULL_HANDLE;
		VkDeviceMemory m_mem = VK_NULL_HANDLE;
		VkImageView m_view = VK_NULL_HANDLE;
		VkSampler m_sampler = VK_NULL_HANDLE;

		void Destroy(VkDevice Device);

		void Load(const std::string& Filename);

		void Load(unsigned int BufferSize, void* pImageData);

	private:

		VulkanCore* m_pVulkanCore = NULL;

		int m_imageWidth = 0;
		int m_imageHeight = 0;
		int m_imageBPP = 0;
	};

}

typedef Engine::VulkanTexture BaseTexture;
typedef Engine::VulkanTexture Texture;