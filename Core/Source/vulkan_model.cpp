#include <vulkan/vulkan.h>

#include "vulkan_core.h"
#include "vulkan_model.h"
#include "vulkan_graphics_pipeline.h"

namespace Engine {


#define UNIFORM_BUFFER_SIZE sizeof(glm::mat4)


	void VkModel::Destroy()
	{
		m_vb.Destroy(m_pVulkanCore->GetDevice());
		m_ib.Destroy(m_pVulkanCore->GetDevice());

		for (int i = 0; i < m_uniformBuffers.size(); i++) {
			m_uniformBuffers[i].Destroy(m_pVulkanCore->GetDevice());
		}
	}


	Texture* VkModel::AllocTexture2D()
	{
		assert(m_pVulkanCore);

		return new VulkanTexture(m_pVulkanCore);
	}


	void VkModel::PopulateBuffers(std::vector<Vertex>& Vertices)
	{
		m_vb = m_pVulkanCore->CreateVertexBuffer(Vertices.data(), ARRAY_SIZE_IN_BYTES(Vertices));

		m_ib = m_pVulkanCore->CreateVertexBuffer(m_Indices.data(), ARRAY_SIZE_IN_BYTES(m_Indices));

		m_uniformBuffers = m_pVulkanCore->CreateUniformBuffers(UNIFORM_BUFFER_SIZE * m_Meshes.size());

		m_vertexSize = sizeof(Vertex);
	}


	void VkModel::CreateDescriptorSets(GraphicsPipeline& Pipeline)
	{
		int NumSubmeshes = (int)m_Meshes.size();
		Pipeline.AllocateDescriptorSets(NumSubmeshes, m_descriptorSets);

		ModelDesc md;

		UpdateModelDesc(md);

		Pipeline.UpdateDescriptorSets(md, m_descriptorSets);
	}


	void VkModel::UpdateModelDesc(ModelDesc& md)
	{
		md.m_vb = m_vb.m_buffer;
		md.m_ib = m_ib.m_buffer;

		md.m_uniforms.resize(m_pVulkanCore->GetNumImages());

		for (int ImageIndex = 0; ImageIndex < m_pVulkanCore->GetNumImages(); ImageIndex++) {
			md.m_uniforms[ImageIndex] = m_uniformBuffers[ImageIndex].m_buffer;
		}

		md.m_materials.resize(m_Meshes.size());
		md.m_ranges.resize(m_Meshes.size());

		int NumSubmeshes = (int)m_Meshes.size();

		for (int SubmeshIndex = 0; SubmeshIndex < NumSubmeshes; SubmeshIndex++) {
			int MaterialIndex = m_Meshes[SubmeshIndex].MaterialIndex;

			if ((MaterialIndex >= 0) && (m_Materials[MaterialIndex].pDiffuse)) {
				Texture* pDiffuse = m_Materials[MaterialIndex].pDiffuse;
				md.m_materials[SubmeshIndex].m_sampler = pDiffuse->m_sampler;
				md.m_materials[SubmeshIndex].m_imageView = pDiffuse->m_view;
			}
			else {
				printf("No diffuse texture in material %d\n", MaterialIndex);
				exit(0);
			}

			size_t offset = m_Meshes[SubmeshIndex].BaseVertex * m_vertexSize;
			size_t range = m_Meshes[SubmeshIndex].NumVertices * m_vertexSize;
			md.m_ranges[SubmeshIndex].m_vbRange = { .m_offset = offset, .m_range = range };

			offset = m_Meshes[SubmeshIndex].BaseIndex * sizeof(uint32_t);
			range = m_Meshes[SubmeshIndex].NumIndices * sizeof(uint32_t);
			md.m_ranges[SubmeshIndex].m_ibRange = { .m_offset = offset, .m_range = range };

			offset = SubmeshIndex * UNIFORM_BUFFER_SIZE;
			range = UNIFORM_BUFFER_SIZE;
			md.m_ranges[SubmeshIndex].m_uniformRange = { .m_offset = offset, .m_range = range };
		}
	}


	void VkModel::RecordCommandBuffer(VkCommandBuffer CmdBuf, GraphicsPipeline& Pipeline, int ImageIndex)
	{
		uint32_t InstanceCount = 1;
		uint32_t FirstInstance = 0;
		uint32_t BaseVertex = 0;

		for (uint32_t SubmeshIndex = 0; SubmeshIndex < m_Meshes.size(); SubmeshIndex++) {
			vkCmdBindDescriptorSets(CmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS,
				Pipeline.GetPipelineLayout(),
				0,  // firstSet
				1,  // descriptorSetCount
				&m_descriptorSets[ImageIndex][SubmeshIndex],
				0,	// dynamicOffsetCount
				NULL);	// pDynamicOffsets

			vkCmdDraw(CmdBuf, m_Meshes[SubmeshIndex].NumIndices,
				InstanceCount, BaseVertex, FirstInstance);
		}
	}


	void VkModel::Update(int ImageIndex, const glm::mat4& Transformation)
	{
		std::vector<glm::mat4> Transformations(m_Meshes.size());

		for (uint32_t SubmeshIndex = 0; SubmeshIndex < Transformations.size(); SubmeshIndex++) {
			glm::mat4 MeshTransform = glm::make_mat4(m_Meshes[SubmeshIndex].Transformation.data());
			MeshTransform = glm::transpose(MeshTransform);
			Transformations[SubmeshIndex] = Transformation * MeshTransform;
		}

		m_uniformBuffers[ImageIndex].Update(m_pVulkanCore->GetDevice(), Transformations.data(), ARRAY_SIZE_IN_BYTES(Transformations));
	}

}