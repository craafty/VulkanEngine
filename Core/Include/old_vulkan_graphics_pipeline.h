#pragma once
#include <vulkan/vulkan.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "vulkan_simple_mesh.h"


namespace Engine {

	class GraphicsPipeline {

	public:

		GraphicsPipeline(VkDevice Device,
			GLFWwindow* pWindow,
			VkRenderPass RenderPass,
			VkShaderModule vs,
			VkShaderModule fs,
			const SimpleMesh* pMesh,
			int NumImages,
			std::vector<BufferAndMemory>& UniformBuffers,
			int UniformDataSize,
			bool DepthEnabled);


		~GraphicsPipeline();

		void Bind(VkCommandBuffer CmdBuf, int ImageIndex);

	private:

		void CreateDescriptorPool(int NumImages);
		void CreateDescriptorSets(const SimpleMesh* pMesh, int NumImages,
			std::vector<BufferAndMemory>& UniformBuffers, int UniformDataSize);
		void CreateDescriptorSetLayout(std::vector<BufferAndMemory>& UniformBuffers, int UniformDataSize, VulkanTexture* pTex);
		void AllocateDescriptorSets(int NumImages);
		void UpdateDescriptorSets(const SimpleMesh* pMesh, int NumImages, std::vector<BufferAndMemory>& UniformBuffers, int UniformDataSize);

		VkDevice m_device = VK_NULL_HANDLE;
		VkPipeline m_pipeline = VK_NULL_HANDLE;
		VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
		VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;
		VkDescriptorSetLayout m_descriptorSetLayout = VK_NULL_HANDLE;
		std::vector<VkDescriptorSet> m_descriptorSets;
	};
}