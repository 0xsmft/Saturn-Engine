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

#include "sppch.h"
#include "Renderer.h"

#include "Saturn/Core/Renderer/RenderThread.h"

#include "VulkanDebug.h"
#include "DescriptorSet.h"
#include "Shader.h"
#include "Framebuffer.h"

#include "Saturn/Core/Profiler.h"

namespace Saturn {

	//////////////////////////////////////////////////////////////////////////

	Renderer::Renderer()
	{
		SingletonStorage::AddSingleton<Renderer>( this );
	}

	Renderer::~Renderer()
	{
		SingletonStorage::RemoveSingleton<Renderer>( this );
	}

	void Renderer::Init()
	{
		// Create Sync objects.
		VkSemaphoreCreateInfo SemaphoreCreateInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
		VkFenceCreateInfo     FenceCreateInfo = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
		FenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		m_FlightFences.resize( MAX_FRAMES_IN_FLIGHT );

		for( int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++ )
		{
			VK_CHECK( vkCreateFence( VulkanContext::Get()->GetDevice(), &FenceCreateInfo, nullptr, &m_FlightFences[ i ] ) );
		}

		VK_CHECK( vkCreateSemaphore( VulkanContext::Get()->GetDevice(), &SemaphoreCreateInfo, nullptr, &m_AcquireSemaphore ) );
		VK_CHECK( vkCreateSemaphore( VulkanContext::Get()->GetDevice(), &SemaphoreCreateInfo, nullptr, &m_SubmitSemaphore ) );

		SetDebugUtilsObjectName( "Acquire Semaphore", ( uint64_t ) m_AcquireSemaphore, VK_OBJECT_TYPE_SEMAPHORE );
		SetDebugUtilsObjectName( "Submit Semaphore", ( uint64_t ) m_SubmitSemaphore, VK_OBJECT_TYPE_SEMAPHORE );

		uint32_t* pData = new uint32_t[ 1 * 1 ];
		std::memset( pData, 0, sizeof( uint32_t ) * 1 * 1 );

		for( uint32_t i = 0; i < 1 * 1; i++ )
		{
			pData[ i ] |= 0xFFFFFFFF;
		}

		// It's really a white texture...
		m_PinkTexture = Ref< Texture2D >::Create( ImageFormat::RGBA8, 1, 1, pData );
		m_PinkTexture->SetIsRendererTexture( true );

		m_PinkTextureCube = Ref< TextureCube >::Create( ImageFormat::BGRA8, 1, 1, pData );
		m_PinkTextureCube->SetIsRendererTexture( true );

		m_PinkTextureSourceAsset = Ref<TextureSourceAsset>::Create();
		m_PinkTextureSourceAsset->m_Texture = m_PinkTexture;

		delete[] pData;

		std::vector<VkDescriptorPoolSize> PoolSizes;
		PoolSizes.emplace_back( VK_DESCRIPTOR_TYPE_SAMPLER, 1000 );
		PoolSizes.emplace_back( VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 );
		PoolSizes.emplace_back( VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 );
		PoolSizes.emplace_back( VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 );
		PoolSizes.emplace_back( VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 );
		PoolSizes.emplace_back( VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 );
		PoolSizes.emplace_back( VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 );
		PoolSizes.emplace_back( VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 );
		PoolSizes.emplace_back( VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 );
		PoolSizes.emplace_back( VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 );

		for( int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++ )
		{
			m_RendererDescriptorPools[ i ] = Ref<DescriptorPool>::Create( PoolSizes, 100000 );
		}
	}

	void Renderer::Terminate()
	{
		// Terminate Semaphores.
		if( m_AcquireSemaphore )
			vkDestroySemaphore( VulkanContext::Get()->GetDevice(), m_AcquireSemaphore, nullptr );

		if( m_SubmitSemaphore )
			vkDestroySemaphore( VulkanContext::Get()->GetDevice(), m_SubmitSemaphore, nullptr );

		m_AcquireSemaphore = nullptr;
		m_SubmitSemaphore = nullptr;

		for( size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++ )
		{
			m_RendererDescriptorSets[ i ] = nullptr;
			m_RendererDescriptorPools[ i ] = nullptr;
		}

		if( m_FlightFences.size() )
		{
			for( int i = 0; i < m_FlightFences.size(); i++ )
			{
				vkDestroyFence( VulkanContext::Get()->GetDevice(), m_FlightFences[ i ], nullptr );
			}
		}

		for( auto& rFunc : m_TerminateResourceFuncs )
			rFunc();

#if !defined(SAT_DIST)
		m_ShaderReloadedCB.clear();
#endif

		m_PinkTextureSourceAsset = nullptr;

		m_PinkTextureCube->SetForceTerminate( true );
		m_PinkTextureCube = nullptr;

		m_PinkTexture->SetForceTerminate( true );
		m_PinkTexture = nullptr;
	}

	void Renderer::SubmitFullscreenQuad(
		VkCommandBuffer CommandBuffer,
		Ref<Saturn::Pipeline> Pipeline,
		Ref<Material> material,
		Ref<IndexBuffer> IndexBuffer, Ref<VertexBuffer> VertexBuffer )
	{
		SAT_PF_EVENT();

		Pipeline->Bind( CommandBuffer );

		material->Bind( CommandBuffer, Pipeline->GetPipelineLayout(), {} );

		VertexBuffer->Bind( CommandBuffer );
		IndexBuffer->Bind( CommandBuffer );

		IndexBuffer->Draw( CommandBuffer );
	}

	void Renderer::SubmitFullscreenQuad2( VkCommandBuffer CommandBuffer, Ref<Saturn::Pipeline> Pipeline, Ref<Material> material )
	{
		Pipeline->Bind( CommandBuffer );
		material->Bind( CommandBuffer, Pipeline->GetPipelineLayout(), {} );

		vkCmdDraw( CommandBuffer, 3, 1, 0, 0 );
	}

	void Renderer::SubmitFullscreenQuadPushConst(
		VkCommandBuffer CommandBuffer, 
		Ref<Saturn::Pipeline> Pipeline, 
		Ref<Material> material, 
		Ref<IndexBuffer> IndexBuffer, 
		Ref<VertexBuffer> VertexBuffer, 
		Buffer PushConstData )
	{
		SAT_PF_EVENT();

		Pipeline->Bind( CommandBuffer );

		SAT_CORE_ASSERT( PushConstData.Data );

		vkCmdPushConstants( CommandBuffer, Pipeline->GetPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, ( uint32_t ) PushConstData.Size, PushConstData.Data );

		material->Bind( CommandBuffer, Pipeline->GetPipelineLayout(), {} );

		VertexBuffer->Bind( CommandBuffer );
		IndexBuffer->Bind( CommandBuffer );

		IndexBuffer->Draw( CommandBuffer );
	}

	void Renderer::BeginRenderPass( VkCommandBuffer CommandBuffer, Pass& rPass )
	{
	}

	void Renderer::EndRenderPass( VkCommandBuffer CommandBuffer )
	{
		vkCmdEndRenderPass( CommandBuffer );
	}

	void Renderer::RenderMeshWithoutMaterial(
		VkCommandBuffer CommandBuffer,
		Ref<Saturn::Pipeline> Pipeline,
		Ref<StaticMesh> mesh,
		Ref<Material> material,
		Ref<UniformBufferSet> ubSet,
		Ref<StorageBufferSet> sbSet,
		uint32_t count,
		Ref<VertexBuffer> transformVB, uint32_t TransformOffset,
		uint32_t SubmeshIndex,
		Buffer additionalData )
	{
		SAT_PF_EVENT();

		Buffer PushConstant;
		PushConstant.Allocate( additionalData.Size );
		if( additionalData.Size > 0 )
			PushConstant.Write( additionalData.Data, additionalData.Size, 0 );

		{
			mesh->GetVertexBuffer()->Bind( CommandBuffer );

			VkDeviceSize offset[ 1 ] = { TransformOffset };
			transformVB->Bind( CommandBuffer, 1, offset );

			mesh->GetIndexBuffer()->Bind( CommandBuffer );

			Pipeline->Bind( CommandBuffer );

			if( PushConstant.Size > 0 )
			{
				vkCmdPushConstants( CommandBuffer, Pipeline->GetPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, ( uint32_t ) PushConstant.Size, PushConstant.Data );
			}

			std::vector<std::vector<VkWriteDescriptorSet>> externalWds;
			if( ubSet )
			{
				externalWds = GetUniformBufferWriteDescriptors( ubSet, material );
				if( sbSet )
				{
					const auto& StorageWriteDescriptors = GetStorageBufferWriteDescriptors( sbSet, material );

					for( size_t i = 0; i < MAX_FRAMES_IN_FLIGHT && StorageWriteDescriptors.size(); i++ )
					{
						// Add StorageWriteDescriptors onto wds
						externalWds[ i ].reserve( externalWds[ i ].size() + StorageWriteDescriptors[ i ].size() );
						externalWds[ i ].insert( externalWds[ i ].end(), StorageWriteDescriptors[ i ].begin(), StorageWriteDescriptors[ i ].end() );
					}
				}
			}

			material->Bind( CommandBuffer, Pipeline->GetPipelineLayout(), externalWds );

			auto& rSubmesh = mesh->Submeshes()[ SubmeshIndex ];
			vkCmdDrawIndexed( CommandBuffer, rSubmesh.IndexCount, count, rSubmesh.BaseIndex, rSubmesh.BaseVertex, 0 );
		}

		PushConstant.Free();
	}

	void Renderer::RenderDynamicMeshWithoutMaterial(
		VkCommandBuffer CommandBuffer,
		Ref<Saturn::Pipeline> Pipeline,
		Ref<SkeletalMesh> mesh,
		Ref<Material> material,
		Ref<UniformBufferSet> ubSet,
		Ref<StorageBufferSet> sbSet,
		uint32_t count,
		Ref<VertexBuffer> transformVB, uint32_t TransformOffset,
		uint32_t SubmeshIndex,
		uint32_t boneOffset, Ref<Material> set2Material, Buffer additionalData )
	{
		SAT_PF_EVENT();

		Buffer PushConstant;
		PushConstant.Allocate( additionalData.Size + sizeof( uint32_t ) );
		if( additionalData.Size > 0 )
			PushConstant.Write( additionalData.Data, additionalData.Size, 0 );

		PushConstant.Write( &boneOffset, sizeof( uint32_t ), ( uint32_t ) additionalData.Size );

		{
			mesh->GetVertexBuffer()->Bind( CommandBuffer );

			VkDeviceSize offset[ 1 ] = { TransformOffset };
			transformVB->Bind( CommandBuffer, 1, offset );

			mesh->GetBoneVertexBuffer()->Bind( CommandBuffer, 2 );

			mesh->GetIndexBuffer()->Bind( CommandBuffer );

			Pipeline->Bind( CommandBuffer );

			if( PushConstant.Size > 0 )
			{
				vkCmdPushConstants( CommandBuffer, Pipeline->GetPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, ( uint32_t ) PushConstant.Size, PushConstant.Data );
			}

			std::vector<std::vector<VkWriteDescriptorSet>> externalWds;
			if( ubSet )
			{
				externalWds = GetUniformBufferWriteDescriptors( ubSet, material );
				if( sbSet )
				{
					const auto& StorageWriteDescriptors = GetStorageBufferWriteDescriptors( sbSet, material );

					for( size_t i = 0; i < MAX_FRAMES_IN_FLIGHT && StorageWriteDescriptors.size(); i++ )
					{
						// Add StorageWriteDescriptors onto wds
						externalWds[ i ].reserve( externalWds[ i ].size() + StorageWriteDescriptors[ i ].size() );
						externalWds[ i ].insert( externalWds[ i ].end(), StorageWriteDescriptors[ i ].begin(), StorageWriteDescriptors[ i ].end() );
					}
				}
			}

			material->Update( externalWds );

			// Descriptor set 0, for material texture data.
			// Descriptor set 1, for environment data.
			std::array<VkDescriptorSet, 2> DescriptorSets = {
				material->GetDescriptorSet( m_FrameCount ),
				set2Material->GetDescriptorSet( m_FrameCount )
			};

			vkCmdBindDescriptorSets( CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
				Pipeline->GetPipelineLayout(), 0, ( uint32_t ) DescriptorSets.size(), DescriptorSets.data(), 0, nullptr );

			auto& rSubmesh = mesh->Submeshes()[ SubmeshIndex ];
			vkCmdDrawIndexed( CommandBuffer, rSubmesh.IndexCount, count, rSubmesh.BaseIndex, rSubmesh.BaseVertex, 0 );
		}

		PushConstant.Free();
	}

	const std::vector<std::vector<VkWriteDescriptorSet>>& Renderer::GetStorageBufferWriteDescriptors( Ref<StorageBufferSet>& rStorageBufferSet, Ref<Material>& rMaterial )
	{
		SAT_PF_EVENT();

		Ref<Shader> shader = rMaterial->GetShader();
		auto shaderHash = shader->GetShaderHash();

		if( m_StorageBufferSets.find( rStorageBufferSet.Get() ) != m_StorageBufferSets.end() )
		{
			const auto& shaderMap = m_StorageBufferSets[ rStorageBufferSet.Get() ];

			if( shaderMap.find( shaderHash ) != shaderMap.end() )
			{
				const auto& wd = shaderMap.at( shaderHash );
				return wd;
			}
		}

		// Does not exist, add and create.
		auto* pDescriptorSet = shader->GetShaderDescriptorSetTemplates( 0 );

		for( auto&& [binding, sb] : pDescriptorSet->StorageBuffers )
		{
			auto& wd = m_StorageBufferSets[ rStorageBufferSet.Get() ][ shaderHash ];
			wd.resize( MAX_FRAMES_IN_FLIGHT );

			for( size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++ )
			{
				Ref<StorageBuffer> ub = rStorageBufferSet->Get( 0u, binding, ( uint32_t ) i );

				VkWriteDescriptorSet wds = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
				wds.descriptorCount = 1;
				wds.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
				wds.pBufferInfo = &ub->GetBufferInfo();
				wds.dstBinding = ub->GetBinding();
				// We don't know the descriptor set yet so we don't set it. It will be updated when we bind the material.

				wd[ i ].push_back( wds );
			}
		}

		return m_StorageBufferSets[ rStorageBufferSet.Get() ][ shaderHash ];
	}

	const std::vector<std::vector<VkWriteDescriptorSet>>& Renderer::GetUniformBufferWriteDescriptors( Ref<UniformBufferSet>& rUniformBufferSet, Ref<Material>& rMaterial )
	{
		Ref<Shader> shader = rMaterial->GetShader();
		auto shaderHash = shader->GetShaderHash();

		if( m_UniformBufferSets.find( rUniformBufferSet.Get() ) != m_UniformBufferSets.end() )
		{
			const auto& shaderMap = m_UniformBufferSets[ rUniformBufferSet.Get() ];

			if( shaderMap.find( shaderHash ) != shaderMap.end() )
			{
				const auto& wd = shaderMap.at( shaderHash );
				return wd;
			}
		}

		// Does not exist, add and create.
		auto* pDescriptorSet = shader->GetShaderDescriptorSetTemplates( 0 );

		for( auto&& [binding, sb] : pDescriptorSet->UniformBuffers )
		{
			auto& wd = m_UniformBufferSets[ rUniformBufferSet.Get() ][ shaderHash ];
			wd.resize( MAX_FRAMES_IN_FLIGHT );

			for( size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++ )
			{
				Ref<UniformBuffer> ub = rUniformBufferSet->Get( 0u, binding, ( uint32_t ) i );

				VkWriteDescriptorSet wds = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
				wds.descriptorCount = 1;
				wds.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				wds.pBufferInfo = &ub->GetBufferInfo();
				wds.dstBinding = ub->GetBinding();
				// We don't know the descriptor set yet so we don't set it. It will be updated when we bind the material.

				wd[ i ].push_back( wds );
			}
		}

		return m_UniformBufferSets[ rUniformBufferSet.Get() ][ shaderHash ];
	}

	void Renderer::SubmitMesh(
		VkCommandBuffer CommandBuffer, Ref< Saturn::Pipeline > Pipeline, Ref< StaticMesh > mesh,
		Ref<StorageBufferSet>& rStorageBufferSet, Ref<UniformBufferSet> rUniformBufferSet, Ref< MaterialRegistry > materialRegistry,
		uint32_t SubmeshIndex, uint32_t count, Ref<VertexBuffer> transformData, uint32_t transformOffset )
	{
		SAT_PF_EVENT();

		VkDeviceSize transformOffsets[ 1 ] = { transformOffset };

		mesh->GetVertexBuffer()->Bind( CommandBuffer );
		transformData->Bind( CommandBuffer, 1, transformOffsets );

		mesh->GetIndexBuffer()->Bind( CommandBuffer );
		Pipeline->Bind( CommandBuffer );

		{
			Submesh& rSubmesh = mesh->Submeshes()[ SubmeshIndex ];
			SAT_CORE_ASSERT( rSubmesh.MaterialIndex < materialRegistry->GetMaterialAssets().size() );
			auto& rMaterialAsset = materialRegistry->GetMaterialAssets()[ rSubmesh.MaterialIndex ];
			Ref<Material> mat = rMaterialAsset->GetMaterial();

			auto wds = GetUniformBufferWriteDescriptors( rUniformBufferSet, mat );
			if( rStorageBufferSet )
			{
				const auto& StorageWriteDescriptors = GetStorageBufferWriteDescriptors( rStorageBufferSet, mat );

				for( size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i )
				{
					// Add StorageWriteDescriptors onto wds
					wds[ i ].reserve( wds[ i ].size() + StorageWriteDescriptors[ i ].size() );
					wds[ i ].insert( wds[ i ].end(), StorageWriteDescriptors[ i ].begin(), StorageWriteDescriptors[ i ].end() );
				}
			}

			rMaterialAsset->RT_Update( wds );

			VkDescriptorSet Set = rMaterialAsset->GetMaterial()->GetDescriptorSet( m_FrameCount );

			vkCmdPushConstants( CommandBuffer, Pipeline->GetPipelineLayout(), VK_SHADER_STAGE_FRAGMENT_BIT, 0, ( uint32_t ) rMaterialAsset->GetPushConstantData().Size, rMaterialAsset->GetPushConstantData().Data );

			// Descriptor set 0, for material texture data.
			// Descriptor set 1, for environment data.
			std::array<VkDescriptorSet, 2> DescriptorSets = {
				Set,
				m_RendererDescriptorSets[ m_FrameCount ]
			};

			vkCmdBindDescriptorSets( CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
				Pipeline->GetPipelineLayout(), 0, ( uint32_t ) DescriptorSets.size(), DescriptorSets.data(), 0, nullptr );

			vkCmdDrawIndexed( CommandBuffer, rSubmesh.IndexCount, count, rSubmesh.BaseIndex, rSubmesh.BaseVertex, 0 );
		}
	}

	void Renderer::SubmitDynamicMesh( VkCommandBuffer CommandBuffer, Ref<Saturn::Pipeline> Pipeline, Ref<SkeletalMesh> mesh, Ref<StorageBufferSet>& rStorageBufferSet, Ref<UniformBufferSet> rUniformBufferSet, Ref< MaterialRegistry > materialRegistry, uint32_t SubmeshIndex, uint32_t count, Ref<VertexBuffer> transformData, uint32_t transformOffset, uint32_t boneOffset, Ref<Material> dynamicMeshMaterial )
	{
		SAT_PF_EVENT();

		VkDeviceSize transformOffsets[ 1 ] = { transformOffset };

		// 0 - Mesh Vertex Data
		mesh->GetVertexBuffer()->Bind( CommandBuffer );
		// 1 - Transform Instance data
		transformData->Bind( CommandBuffer, 1, transformOffsets );
		// 2 - Mesh Bone Data
		mesh->GetBoneVertexBuffer()->Bind( CommandBuffer, 2 );

		mesh->GetIndexBuffer()->Bind( CommandBuffer );
		Pipeline->Bind( CommandBuffer );

		auto frame = Renderer::Get()->GetCurrentFrame();

		{
			Submesh& rSubmesh = mesh->Submeshes()[ SubmeshIndex ];
			auto& rMaterialAsset = materialRegistry->GetMaterialAssets()[ rSubmesh.MaterialIndex ];
			Ref<Material> mat = rMaterialAsset->GetMaterial();

			auto wds = GetUniformBufferWriteDescriptors( rUniformBufferSet, mat );
			if( rStorageBufferSet )
			{
				const auto& StorageWriteDescriptors = GetStorageBufferWriteDescriptors( rStorageBufferSet, mat );

				for( size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++ )
				{
					// Add StorageWriteDescriptors onto wds
					wds[ i ].reserve( wds[ i ].size() + StorageWriteDescriptors[ i ].size() );
					wds[ i ].insert( wds[ i ].end(), StorageWriteDescriptors[ i ].begin(), StorageWriteDescriptors[ i ].end() );
				}
			}

			rMaterialAsset->RT_Update( wds );

			// Bone index vertex push const
			vkCmdPushConstants( 
				CommandBuffer, 
				Pipeline->GetPipelineLayout(), 
				VK_SHADER_STAGE_VERTEX_BIT, 
				0, 
				sizeof( uint32_t ), 
				&boneOffset 
			);

			// Material data push const, fragment
			vkCmdPushConstants(
				CommandBuffer,
				Pipeline->GetPipelineLayout(),
				VK_SHADER_STAGE_FRAGMENT_BIT,
				16, // This offset is really really retarded... In the shader the offset of the first element is 16, when I remove the offset Vulkan complains saying that it overlaps, but when I set it to 16 it works... WTF
				( uint32_t ) rMaterialAsset->GetPushConstantData().Size,
				rMaterialAsset->GetPushConstantData().Data 
			);

			VkDescriptorSet Set = rMaterialAsset->GetMaterial()->GetDescriptorSet( m_FrameCount );

			// Descriptor set 0, for material texture data.
			// Descriptor set 1, for environment data.
			// Descriptor set 2, bone vertex buffer data.
			std::array<VkDescriptorSet, 3> DescriptorSets = {
				Set,
				m_RendererDescriptorSets[ m_FrameCount ],
				dynamicMeshMaterial->GetDescriptorSet( m_FrameCount )
			};

			vkCmdBindDescriptorSets( CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
				Pipeline->GetPipelineLayout(), 0, ( uint32_t ) DescriptorSets.size(), DescriptorSets.data(), 0, nullptr );

			vkCmdDrawIndexed( CommandBuffer, rSubmesh.IndexCount, count, rSubmesh.BaseIndex, rSubmesh.BaseVertex, 0 );
		}
	}

	void Renderer::SetSceneEnvironment( Ref<Image2D> ShadowMap, Ref<EnvironmentMap> Environment, Ref<Texture2D> BRDF )
	{
		SAT_PF_EVENT();

		Ref<Shader> shader = ShaderLibrary::Get().Find( "shader_new" );

		m_RendererDescriptorSets[ m_FrameCount ] = shader->AllocateDescriptorSet( 1, true );

		shader->WriteDescriptor( "u_ShadowMap", ShadowMap->GetDescriptorInfo(), m_RendererDescriptorSets[ m_FrameCount ] );
		shader->WriteDescriptor( "u_BRDFLUTTexture", BRDF->GetDescriptorInfo(), m_RendererDescriptorSets[ m_FrameCount ] );

		if( Environment && Environment->RadianceMap && Environment->IrradianceMap )
		{
			shader->WriteDescriptor( "u_EnvRadianceTex", Environment->RadianceMap->GetDescriptorInfo(), m_RendererDescriptorSets[ m_FrameCount ] );
			shader->WriteDescriptor( "u_EnvIrradianceTex", Environment->IrradianceMap->GetDescriptorInfo(), m_RendererDescriptorSets[ m_FrameCount ] );
		}
		else
		{
			shader->WriteDescriptor( "u_EnvRadianceTex", m_PinkTextureCube->GetDescriptorInfo(), m_RendererDescriptorSets[ m_FrameCount ] );
			shader->WriteDescriptor( "u_EnvIrradianceTex", m_PinkTextureCube->GetDescriptorInfo(), m_RendererDescriptorSets[ m_FrameCount ] );
		}
	}

	VkCommandBuffer Renderer::AllocateCommandBuffer( VkCommandPool CommandPool )
	{
		SAT_PF_EVENT();

		VkCommandBufferAllocateInfo AllocateInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
		AllocateInfo.commandPool = VulkanContext::Get()->GetCommandPool();
		AllocateInfo.commandBufferCount = 1;
		AllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

		VkCommandBuffer CommandBuffer;
		VK_CHECK( vkAllocateCommandBuffers( VulkanContext::Get()->GetDevice(), &AllocateInfo, &CommandBuffer ) );

		VkCommandBufferBeginInfo CommandPoolBeginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
		CommandPoolBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		VK_CHECK( vkBeginCommandBuffer( CommandBuffer, &CommandPoolBeginInfo ) );

		return CommandBuffer;
	}

	VkCommandBuffer Renderer::AllocateCommandBuffer( VkCommandBufferLevel CmdLevel )
	{
		SAT_PF_EVENT();

		VkCommandBufferAllocateInfo AllocateInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
		AllocateInfo.commandPool = VulkanContext::Get()->GetCommandPool();
		AllocateInfo.commandBufferCount = 1;
		AllocateInfo.level = CmdLevel;

		VkCommandBuffer CommandBuffer;
		VK_CHECK( vkAllocateCommandBuffers( VulkanContext::Get()->GetDevice(), &AllocateInfo, &CommandBuffer ) );

		return CommandBuffer;
	}

	void Renderer::BeginFrame()
	{
		SAT_PF_EVENT();

		VkDevice LogicalDevice = VulkanContext::Get()->GetDevice();

		m_BeginFrameTimer.Reset();

		VK_CHECK( vkResetDescriptorPool( LogicalDevice, m_RendererDescriptorPools[ m_FrameCount ]->GetVulkanPool(), 0 ) );

#if !defined(SAT_DIST)
		if( m_PendingShaderReloads.size() )
		{
			for( const std::string& rName : m_PendingShaderReloads )
			{
				for( auto& rFunction : m_ShaderReloadedCB )
					rFunction( rName );
			}

			m_PendingShaderReloads.clear();
		}
#endif

		// ^^^ cleanup from last frame
		// Actual Begin frame vvvv

		m_CommandBuffer = AllocateCommandBuffer( VulkanContext::Get()->GetCommandPool() );

		// Wait for last frame.
		VK_CHECK( vkWaitForFences( LogicalDevice, 1, &m_FlightFences[ m_FrameCount ], VK_TRUE, UINT32_MAX ) );

		// Reset current fence.
		VK_CHECK( vkResetFences( LogicalDevice, 1, &m_FlightFences[ m_FrameCount ] ) );

		// Acquire next image.
		uint32_t ImageIndex = UINT32_MAX;
		VulkanContext::Get()->GetSwapchain().AcquireNextImage( UINT32_MAX, m_AcquireSemaphore, VK_NULL_HANDLE, &ImageIndex );

		m_ImageIndex = ImageIndex;

		SAT_CORE_ASSERT( ImageIndex != UINT32_MAX );

		m_BeginFrameTime = m_BeginFrameTimer.ElapsedMilliseconds();
	}

	void Renderer::EndFrame()
	{
		SAT_PF_EVENT();

		m_EndFrameTimer.Reset();

		VkDevice LogicalDevice = VulkanContext::Get()->GetDevice();

		VK_CHECK( vkEndCommandBuffer( m_CommandBuffer ) );

		// Rendering Queue
		VkPipelineStageFlags WaitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

		VkSubmitInfo SubmitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
		SubmitInfo.commandBufferCount = 1;
		SubmitInfo.pCommandBuffers = &m_CommandBuffer;
		SubmitInfo.pWaitDstStageMask = &WaitStage;

		// SIGNAL the SubmitSemaphore
		SubmitInfo.pSignalSemaphores = &m_SubmitSemaphore;
		SubmitInfo.signalSemaphoreCount = 1;

		// WAIT for AcquireSemaphore
		SubmitInfo.pWaitSemaphores = &m_AcquireSemaphore;
		SubmitInfo.waitSemaphoreCount = 1;

		VK_CHECK( vkResetFences( LogicalDevice, 1, &m_FlightFences[ m_FrameCount ] ) );

		// Use current fence to be signaled.
		VK_CHECK( vkQueueSubmit( VulkanContext::Get()->GetGraphicsQueue(), 1, &SubmitInfo, m_FlightFences[ m_FrameCount ] ) );

		// Present info.
		VkPresentInfoKHR PresentInfo = { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
		PresentInfo.pSwapchains = &VulkanContext::Get()->GetSwapchain().GetSwapchain();
		PresentInfo.swapchainCount = 1;
		PresentInfo.pImageIndices = &m_ImageIndex;

		// WAIT for SubmitSemaphore
		PresentInfo.pWaitSemaphores = &m_SubmitSemaphore;
		PresentInfo.waitSemaphoreCount = 1;

		{
			SAT_PF_EVENT_N( "Saturn::Renderer::QueuePresent" );

			m_QueuePresentTimer.Reset();

			VkResult Result = vkQueuePresentKHR( VulkanContext::Get()->GetGraphicsQueue(), &PresentInfo );

			if( Result == VK_ERROR_OUT_OF_DATE_KHR )
			{
				SAT_CORE_INFO( "Result was VK_ERROR_OUT_OF_DATE_KHR, Swapchain will be re-created!" );

				VulkanContext::Get()->GetSwapchain().Recreate();

				PresentInfo.pSwapchains = &VulkanContext::Get()->GetSwapchain().GetSwapchain();

				VK_CHECK( vkQueuePresentKHR( VulkanContext::Get()->GetGraphicsQueue(), &PresentInfo ) );
			}

			m_QueuePresentTime = m_QueuePresentTimer.ElapsedMilliseconds();
		}

		{
			m_QueueWaitTimer.Reset();
			SAT_PF_EVENT_N( "Saturn::Renderer::QueueWait" );
			VK_CHECK( vkQueueWaitIdle( VulkanContext::Get()->GetPresentQueue() ) );
			m_QueueWaitTimer.Stop();
			m_QueueWaitTime = m_QueuePresentTimer.ElapsedMilliseconds();
		}

		vkFreeCommandBuffers( LogicalDevice, VulkanContext::Get()->GetCommandPool(), 1, &m_CommandBuffer );

		m_FrameCount = ( m_FrameCount + 1 ) % MAX_FRAMES_IN_FLIGHT;

		m_EndFrameTime = m_QueueWaitTime + ( m_EndFrameTimer.ElapsedMilliseconds() + m_QueuePresentTime );

		// Clear storage buffer sets. Reallocated next frame.
		// Not ideal but for now we will do this as in the LightCulling pass we resize the buffer every frame meaning we have to update our cache.
		m_StorageBufferSets.clear();
		m_UniformBufferSets.clear();
	}

	void Renderer::SubmitTerminateResource( std::function<void()>&& rrFunction )
	{
		m_TerminateResourceFuncs.push_back( rrFunction );
	}

	std::pair< Ref<VertexBuffer>, Ref<IndexBuffer> > Renderer::CreateFullscreenQuad()
	{
		Ref<VertexBuffer> vertex = nullptr;
		Ref<IndexBuffer> index = nullptr;

		struct QuadVertex
		{
			glm::vec3 Position;
			glm::vec2 TexCoord;
		};

		constexpr float x = -1;
		constexpr float y = -1;

		constexpr float width = 2, height = 2;

		QuadVertex* data = new QuadVertex[ 4 ];

		data[ 0 ].Position = glm::vec3( x, y, 0.0f );
		data[ 0 ].TexCoord = glm::vec2( 0, 0 );

		data[ 1 ].Position = glm::vec3( x + width, y, 0.0f );
		data[ 1 ].TexCoord = glm::vec2( 1, 0 );

		data[ 2 ].Position = glm::vec3( x + width, y + height, 0.0f );
		data[ 2 ].TexCoord = glm::vec2( 1, 1 );

		data[ 3 ].Position = glm::vec3( x, y + height, 0.0f );
		data[ 3 ].TexCoord = glm::vec2( 0, 1 );

		vertex = Ref<VertexBuffer>::Create( data, 4 * sizeof( QuadVertex ) );

		uint32_t indices[ 6 ] = { 0, 1, 2,
								  2, 3, 0, };

		index = Ref<IndexBuffer>::Create( indices, 6 * sizeof( uint32_t ) );

		delete[] data;

		return { vertex, index };
	}

#if !defined(SAT_DIST)
	void Renderer::AddShaderReloadCB( const std::function<void( const std::string& )>& rFunc )
	{
		m_ShaderReloadedCB.push_back( rFunc );
	}

	void Renderer::OnShaderReloaded( const std::string& rName )
	{
		m_PendingShaderReloads.push_back( rName );
	}

	void Renderer::AddShaderReference( UUID Hash )
	{
		m_ShaderReferences[ Hash ] = { .Hash = Hash };
	}

	void Renderer::RemoveShaderReference( UUID Hash )
	{
		m_ShaderReferences.erase( Hash );
	}

	void Renderer::ClearShaderReferences()
	{
		m_ShaderReferences.clear();
	}

	void Renderer::RemovePipelineReferenceFromShaderRef( UUID Hash, Pipeline* pPipeline )
	{
		const auto itr = m_ShaderReferences.find( Hash );
		if( itr != m_ShaderReferences.end() )
		{
			auto& rPipelines = itr->second.Pipelines;

			const auto pipelineItr = std::find( rPipelines.begin(), rPipelines.end(), pPipeline );
			if( pipelineItr != rPipelines.end() )
			{
				rPipelines.erase( pipelineItr );
			}
		}
	}

	void Renderer::RemoveMaterialReferenceFromShaderRef( UUID Hash, Material* pMaterial )
	{
		const auto itr = m_ShaderReferences.find( Hash );
		if( itr != m_ShaderReferences.end() )
		{
			auto& rMaterials = itr->second.Materials;

			const auto materialItr = std::find( rMaterials.begin(), rMaterials.end(), pMaterial );
			if( materialItr != rMaterials.end() )
			{
				rMaterials.erase( materialItr );
			}
		}
	}

	ShaderReference& Renderer::FindOrCreateShaderReference( UUID Hash )
	{
		return m_ShaderReferences[ Hash ];
	}
#endif

}
