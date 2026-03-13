/********************************************************************************************
*                                                                                           *
*                                                                                           *
*                                                                                           *
* MIT License                                                                               *
*                                                                                           *
* Copyright (c) 2020 - 2026 BEAST                                                           *
*                                                                                           *
* Permission is hereby granted, free of charge, to any person obtaining a copy              *
* of this software and associated documentation files (the "Software"), to deal             *
* in the Software without restriction, including without limitation the rights              *
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell                 *
* copies of the Software, and to permit persons to whom the Software is                     *
* furnished to do so, subject to the following conditions:                                  *
*                                                                                           *
* The above copyright notice and this permission notice shall be included in all            *
* copies or substantial portions of the Software.                                           *
*                                                                                           *
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR                *
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,                  *
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE               *
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER                    *
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,             *
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE             *
* SOFTWARE.                                                                                 *
*********************************************************************************************
*/

#pragma once

#include "VulkanContext.h"
#include "EnvironmentMap.h"
#include "StorageBufferSet.h"
#include "UniformBufferSet.h"

// #FixRendererIncludeToTSA
#include "Saturn/Asset/TextureSourceAsset.h"

namespace Saturn {

	struct ShaderReference
	{
		UUID Hash = 0;

		std::vector<Ref<Pipeline>> Pipelines;
		std::vector<Ref<Material>> Materials;
		//std::vector<Ref<MaterialAssets>> MaterialAssets;

		~ShaderReference() 
		{
			Pipelines.clear();
			Materials.clear();
		}
	};

	class Renderer : public RefTarget
	{
	public:
		static inline Renderer* Get() { return SingletonStorage::GetSingleton<Renderer>(); }
	public:
		Renderer();
		~Renderer();

		void SubmitFullscreenQuad( VkCommandBuffer CommandBuffer, Ref<Saturn::Pipeline> Pipeline, Ref<Material> material, Ref<UniformBufferSet> ubSet, Ref<IndexBuffer> IndexBuffer, Ref<VertexBuffer> VertexBuffer );

		// Render pass helpers.
		void BeginRenderPass( VkCommandBuffer CommandBuffer, Pass& rPass );
		void EndRenderPass( VkCommandBuffer CommandBuffer );

		void RenderMeshWithoutMaterial( VkCommandBuffer CommandBuffer, Ref<Saturn::Pipeline> Pipeline, Ref<StaticMesh> mesh, Ref<Material> material, Ref<UniformBufferSet> ubSet,
			Ref<StorageBufferSet> sbSet, uint32_t count, Ref<VertexBuffer> transformVB, uint32_t TransformOffset, uint32_t SubmeshIndex, Buffer additionalData = Buffer() );

		void RenderDynamicMeshWithoutMaterial( VkCommandBuffer CommandBuffer, Ref<Saturn::Pipeline> Pipeline, Ref<SkeletalMesh> mesh, Ref<Material> material, Ref<UniformBufferSet> ubSet, Ref<StorageBufferSet> sbSet, uint32_t count, Ref<VertexBuffer> transformVB, uint32_t TransformOffset, uint32_t SubmeshIndex, uint32_t boneOffset, Ref<Material> set2Material, Buffer additionalData = Buffer() );
	
		void SubmitMesh( VkCommandBuffer CommandBuffer, Ref< Saturn::Pipeline > Pipeline, Ref< StaticMesh > mesh,
			Ref<StorageBufferSet>& rStorageBufferSet, Ref<UniformBufferSet> rUniformBufferSet, Ref< MaterialRegistry > materialRegistry, uint32_t SubmeshIndex, uint32_t count,
			Ref<VertexBuffer> transformData, uint32_t transformOffset );

		void SubmitDynamicMesh( VkCommandBuffer CommandBuffer, Ref<Saturn::Pipeline> Pipeline, Ref<SkeletalMesh> mesh,
			Ref<StorageBufferSet>& rStorageBufferSet, Ref<UniformBufferSet> rUniformBufferSet, Ref< MaterialRegistry > materialRegistry, uint32_t SubmeshIndex, uint32_t count,
			Ref<VertexBuffer> transformData, uint32_t transformOffset, uint32_t boneOffset, Ref<Material> dynamicMeshMaterial );

		const std::vector<std::vector<VkWriteDescriptorSet>>& GetStorageBufferWriteDescriptors( Ref<StorageBufferSet>& rStorageBufferSet, Ref<Material>& rMaterialAsset );

		const std::vector<std::vector<VkWriteDescriptorSet>>& GetUniformBufferWriteDescriptors( Ref<UniformBufferSet>& rUniformBufferSet, Ref<Material>& rMaterialAsset );

		void SetSceneEnvironment( Ref<Image2D> ShadowMap, Ref<EnvironmentMap> Environment, Ref<Texture2D> BRDF );

		// Allocate command buffer.
		VkCommandBuffer AllocateCommandBuffer( VkCommandPool CommandPool );
		VkCommandBuffer AllocateCommandBuffer( VkCommandBufferLevel CmdLevel );

		//////////////////////////////////////////////////////////////////////////
		// FRAME BEGINGING AND ENDING.
		//////////////////////////////////////////////////////////////////////////
		
		void BeginFrame();
		void EndFrame();
		
		uint32_t GetImageIndex()   const { return m_ImageIndex; }
		uint32_t GetCurrentFrame() const { return m_FrameCount; }

		std::pair< float, float > GetFrameTimings() { return std::make_pair( m_BeginFrameTime, m_EndFrameTime ); }
		float GetQueuePresentTime() const { return m_QueuePresentTime; }
		float GetQueueWaitTime() const { return m_QueuePresentTime; }

		void SubmitTerminateResource( std::function<void()>&& rrFunction );

		Ref<Texture2D>          GetPinkTexture() { return m_PinkTexture; }
		Ref<TextureCube>        GetPinkTextureCube() { return m_PinkTextureCube; }
		Ref<TextureSourceAsset> GetPinkTextureSrcAsset() { return m_PinkTextureSourceAsset; }

		std::pair< Ref<VertexBuffer>, Ref<IndexBuffer>> CreateFullscreenQuad();
		
		Ref<DescriptorPool> GetDescriptorPool() { return m_RendererDescriptorPools[ m_FrameCount ]; }

#if !defined(SAT_DIST)
		void AddShaderReloadCB( const std::function<void( const std::string& )>& rFunc );
		void OnShaderReloaded( const std::string& rName );

		void AddShaderReference( UUID Hash );
		void RemoveShaderReference( UUID Hash );
		void ClearShaderReferences();

		ShaderReference& FindShaderReference( UUID Hash );
#endif

	public:
		VkCommandBuffer ActiveCommandBuffer() const { return m_CommandBuffer; };

	private:
		void Init();
		void Terminate();

	private:
		uint32_t m_ImageIndex = 0;
		uint32_t m_ImageCount = 0;
		uint32_t m_FrameCount = 0;

		float m_BeginFrameTime   = 0.0f;
		float m_EndFrameTime     = 0.0f;
		float m_QueuePresentTime = 0.0f;
		float m_QueueWaitTime    = 0.0f;

		Timer m_BeginFrameTimer;
		Timer m_EndFrameTimer;
		Timer m_QueuePresentTimer;
		Timer m_QueueWaitTimer;

		std::vector<VkFence> m_FlightFences;
		
		std::vector< std::function<void()> > m_TerminateResourceFuncs;
		
		VkSemaphore m_AcquireSemaphore = nullptr;
		VkSemaphore m_SubmitSemaphore = nullptr;

		VkCommandBuffer m_CommandBuffer = nullptr;
		
		Ref<Texture2D> m_PinkTexture;
		Ref<TextureCube> m_PinkTextureCube;
		Ref<TextureSourceAsset> m_PinkTextureSourceAsset;

		VkDescriptorSet m_RendererDescriptorSets[ MAX_FRAMES_IN_FLIGHT ]{};

		Ref<DescriptorPool> m_RendererDescriptorPools[ MAX_FRAMES_IN_FLIGHT ];

		// frame -> shader name -> set
		std::unordered_map<StorageBufferSet*, std::unordered_map<UUID, std::vector<std::vector<VkWriteDescriptorSet>>>> m_StorageBufferSets;

		// ub ptr -> shader UUIDs -> Frame -> set
		std::unordered_map<UniformBufferSet*, std::unordered_map<UUID, std::vector<std::vector<VkWriteDescriptorSet>>>> m_UniformBufferSets;

#if !defined(SAT_DIST)
		std::vector< std::function<void( const std::string& )> > m_ShaderReloadedCB;
		std::unordered_map<size_t, ShaderReference> m_ShaderReferences;
		std::vector<std::string> m_PendingShaderReloads;
#endif

	private:
		friend class VulkanContext;
	};
}