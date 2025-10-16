#pragma once

#include <stdio.h>
#include <stdlib.h>

#include <vulkan/vulkan.h>

#include "vulkan_core.h"

namespace Engine {

	struct SimpleMesh {
		BufferAndMemory m_vb;
		size_t m_vertexBufferSize = 0;
		VulkanTexture* m_pTex = NULL;

		void Destroy(VkDevice Device)
		{
			m_vb.Destroy(Device);

			if (m_pTex) {
				m_pTex->Destroy(Device);
				delete m_pTex;
			}
		}
	};

}