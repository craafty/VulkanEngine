#pragma once
#include <vulkan/vulkan.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "vulkan_core.h"
#include "Int/model_desc.h"

namespace Engine {

	class GraphicsPipeline {

	public:

		GraphicsPipeline(VkDevice Device,
			GLFWwindow* pWindow,
			VkRenderPass RenderPass,
			VkShaderModule vs,
			VkShaderModule fs,
			int NumImages);

		~GraphicsPipeline();

		void Bind(VkCommandBuffer CmdBuf);

		void AllocateDescriptorSets(int NumSubmeshes, std::vector< std::vector<VkDescriptorSet> >& DescriptorSets);

		void UpdateDescriptorSets(const ModelDesc& ModelDesc,
			std::vector<std::vector<VkDescriptorSet>>& DescriptorSets);

		VkPipelineLayout GetPipelineLayout() const { return m_pipelineLayout; }

	private:

		void InitCommon(GLFWwindow* pWindow, VkRenderPass RenderPass, VkShaderModule vs, VkShaderModule fs);

		void AllocateDescriptorSetsInternal(int NumSubmeshes, std::vector< std::vector<VkDescriptorSet> >& DescriptorSets);
		void CreateDescriptorPool(int MaxSets);
		void CreateDescriptorSetLayout(bool IsVB, bool IsIB, bool IsTex, bool IsUniform);

		VkDevice m_device = VK_NULL_HANDLE;
		VkPipeline m_pipeline = VK_NULL_HANDLE;
		VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
		VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;
		VkDescriptorSetLayout m_descriptorSetLayout = VK_NULL_HANDLE;
		int m_numImages = 0;
	};

}