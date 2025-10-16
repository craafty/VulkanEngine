#include <string>
#include <assert.h>

#include <vulkan/vulkan.h>

#include "vulkan_core.h"
#include "vulkan_texture.h"
#include "stb/stb_image.h"
#include "stb/stb_image_write.h"


namespace Engine {

	void VulkanTexture::Load(unsigned int BufferSize, void* pData)
	{
		assert(m_pVulkanCore);

		void* pImageData = stbi_load_from_memory((const stbi_uc*)pData, BufferSize, &m_imageWidth, &m_imageHeight, &m_imageBPP, 0);

		m_pVulkanCore->CreateTextureFromData(pImageData, m_imageWidth, m_imageHeight, *this);
	}


	void VulkanTexture::Load(const std::string& Filename)
	{
		m_pVulkanCore->CreateTexture(Filename.c_str(), *this);
	}

}