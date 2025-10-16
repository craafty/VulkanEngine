#pragma once

#include <vulkan/vulkan.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "vulkan_util.h"
#include "vulkan_device.h"
#include "vulkan_queue.h"

namespace Engine {


	class BufferAndMemory {
	public:
		BufferAndMemory() {}

		VkBuffer m_buffer = NULL;
		VkDeviceMemory m_mem = NULL;
		VkDeviceSize m_allocationSize = 0;

		void Update(VkDevice Device, const void* pData, size_t Size);

		void Destroy(VkDevice Device);
	};


	class VulkanTexture {
	public:
		VulkanTexture() {}

		VkImage m_image = VK_NULL_HANDLE;
		VkDeviceMemory m_mem = VK_NULL_HANDLE;
		VkImageView m_view = VK_NULL_HANDLE;
		VkSampler m_sampler = VK_NULL_HANDLE;

		void Destroy(VkDevice Device);
	};


	class VulkanCore {

	public:

		VulkanCore();

		~VulkanCore();

		void Init(const char* pAppName, GLFWwindow* pWindow, bool DepthEnabled);

		VkRenderPass CreateSimpleRenderPass();

		std::vector<VkFramebuffer> CreateFramebuffers(VkRenderPass RenderPass) const;

		void DestroyFramebuffers(std::vector<VkFramebuffer>& Framebuffers);

		VkDevice& GetDevice() { return m_device; }

		int GetNumImages() const { return (int)m_images.size(); }

		const VkImage& GetImage(int Index) const;

		VulkanQueue* GetQueue() { return &m_queue; }

		uint32_t GetQueueFamily() const { return m_queueFamily; }

		void CreateCommandBuffers(uint32_t Count, VkCommandBuffer* pCmdBufs);

		void FreeCommandBuffers(uint32_t Count, const VkCommandBuffer* pCmdBufs);

		BufferAndMemory CreateVertexBuffer(const void* pVertices, size_t Size);

		std::vector<BufferAndMemory> CreateUniformBuffers(size_t Size);

		void CreateTexture(const char* filename, VulkanTexture& Tex);

		void CreateTextureFromData(const void* pPixels, int ImageWidth, int ImageHeight, VulkanTexture& Tex);

	private:

		void CreateInstance(const char* pAppName);
		void CreateDebugCallback();
		void CreateSurface();
		void CreateDevice();
		void CreateSwapChain();
		void CreateCommandBufferPool();
		BufferAndMemory CreateUniformBuffer(size_t Size);
		void CreateDepthResources();

		uint32_t GetMemoryTypeIndex(uint32_t memTypeBits, VkMemoryPropertyFlags memPropFlags);

		void CopyBufferToBuffer(VkBuffer Dst, VkBuffer Src, VkDeviceSize Size);

		BufferAndMemory CreateBuffer(VkDeviceSize Size, VkBufferUsageFlags Usage, VkMemoryPropertyFlags Properties);

		void CreateTextureImageFromData(VulkanTexture& Tex, const void* pPixels, uint32_t ImageWidth, uint32_t ImageHeight,
			VkFormat TexFormat);
		void CreateImage(VulkanTexture& Tex, uint32_t ImageWidth, uint32_t ImageHeight, VkFormat TexFormat,
			VkImageUsageFlags UsageFlags, VkMemoryPropertyFlagBits PropertyFlags);
		void UpdateTextureImage(VulkanTexture& Tex, uint32_t ImageWidth, uint32_t ImageHeight, VkFormat TexFormat, const void* pPixels);
		void CopyBufferToImage(VkImage Dst, VkBuffer Src, uint32_t ImageWidth, uint32_t ImageHeight);
		void TransitionImageLayout(VkImage& Image, VkFormat Format, VkImageLayout OldLayout, VkImageLayout NewLayout);
		void SubmitCopyCommand();
		void GetFramebufferSize(int& Width, int& Height) const;

		VkInstance m_instance = VK_NULL_HANDLE;
		VkDebugUtilsMessengerEXT m_debugMessenger = VK_NULL_HANDLE;
		GLFWwindow* m_pWindow = NULL;
		VkSurfaceKHR m_surface = VK_NULL_HANDLE;
		VulkanPhysicalDevices m_physDevices;
		uint32_t m_queueFamily = 0;
		VkDevice m_device = VK_NULL_HANDLE;
		VkSurfaceFormatKHR m_swapChainSurfaceFormat = {};
		VkSwapchainKHR m_swapChain = VK_NULL_HANDLE;
		std::vector<VkImage> m_images;
		std::vector<VkImageView> m_imageViews;
		std::vector<VulkanTexture> m_depthImages;
		VkCommandPool m_cmdBufPool = VK_NULL_HANDLE;
		VulkanQueue m_queue;
		VkCommandBuffer m_copyCmdBuf = VK_NULL_HANDLE;
		int m_windowWidth = 0;
		int m_windowHeight = 0;
		bool m_depthEnabled = false;
	};

}