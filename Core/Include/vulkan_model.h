#pragma once

#include "vulkan_texture.h"
#include "vulkan_graphics_pipeline.h"
#include "core_model.h"
#include "model_desc.h"

namespace Engine {

	class VulkanCore;

	class VkModel : public CoreModel
	{
	public:

		VkModel() {}

		void Destroy();

		void Init(VulkanCore* pVulkanCore) { m_pVulkanCore = pVulkanCore; }

		void CreateDescriptorSets(GraphicsPipeline& Pipeline);

		void RecordCommandBuffer(VkCommandBuffer CmdBuf, GraphicsPipeline& pPipeline, int ImageIndex);

		void Update(int ImageIndex, const glm::mat4& Transformation);

		const BufferAndMemory* GetVB() const { return &m_vb; }

		const BufferAndMemory* GetIB() const { return &m_ib; }

	protected:

		virtual void AllocBuffers() { /* Nothing to do here */ }

		virtual Texture* AllocTexture2D();

		virtual void InitGeometryPost() { /* Nothing to do here */ }

		virtual void PopulateBuffersSkinned(std::vector<SkinnedVertex>& Vertices) { assert(0); }

		virtual void PopulateBuffers(std::vector<Vertex>& Vertices);

	private:
		void UpdateModelDesc(ModelDesc& md);

		VulkanCore* m_pVulkanCore = NULL;

		BufferAndMemory m_vb;
		BufferAndMemory m_ib;
		std::vector<BufferAndMemory> m_uniformBuffers;
		std::vector<std::vector<VkDescriptorSet>> m_descriptorSets;
		size_t m_vertexSize = 0;	// sizeof(Vertex) OR sizeof(SkinnedVertex)
	};

}