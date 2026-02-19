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
#include "Material.h"

#include "Mesh.h"
#include "DescriptorSet.h"
#include "Renderer.h"
#include "Texture.h"
#include "VulkanContext.h"
#include "VulkanDebug.h"

#include "Saturn/Core/Profiler.h"

namespace Saturn {

	Material::Material( const Ref< Saturn::Shader >& rShader, const std::string& rMaterialName, uint32_t set )
		: m_Shader( rShader ), m_Set( set )
	{
		Initialise( rMaterialName );
	}

	void Material::Initialise( const std::string& rMaterialName )
	{
		if( rMaterialName.empty() )
		{
			std::string NewName = std::format( "{0} Unknown Material {1}", m_Shader->GetName(), std::to_string( UUID() ) );
			m_Name = NewName;
		}
		else
			m_Name = rMaterialName;

		InitLayout();
	}

	void Material::InitLayout()
	{
		// We are always set 0
		// Set 1 is owned by the renderer
		ShaderDescriptorSetTemplate* pMaterialDS = m_Shader->GetShaderDescriptorSetTemplates( m_Set );
		if( pMaterialDS )
		{
			// Copy for our own use
			m_DescriptorSetTemplate = ShaderDescriptorSetTemplate( pMaterialDS );
		}

		size_t totalPCSizes = 0;
		for( const auto& rPushConstantBuffers : m_Shader->GetPushConstantBuffer() )
		{
			totalPCSizes += rPushConstantBuffers.Size;
		}

		if( totalPCSizes )
		{
			m_ShaderPC = m_Shader->GetPushConstantBuffer().back();
		
			m_PushConstantData.Allocate( totalPCSizes );
			m_PushConstantData.Zero_Memory();
		}
	}

	Material::~Material()
	{
		m_PushConstantData.Free();
		m_UniformBuffers.clear();

		for( auto& [key, texture] : m_Textures )
		{
			if( !texture )
				continue;

			texture = nullptr;
		}
	}
	
	void Material::Copy( Ref<Material>& rOther )
	{
		m_Textures.clear();
		m_UniformBuffers.clear();

		m_Textures = rOther->m_Textures;
		m_UniformBuffers = rOther->m_UniformBuffers;

		m_Name = rOther->GetName();

		m_Shader = rOther->m_Shader;
		m_PushConstantData = rOther->m_PushConstantData;
		m_ShaderPC = rOther->m_ShaderPC;
		m_DescriptorSetTemplate = rOther->m_DescriptorSetTemplate;
	}

	void Material::Bind( VkCommandBuffer CommandBuffer, VkPipelineLayout Layout, const std::vector<std::vector<VkWriteDescriptorSet>>& rExtraWds, VkPipelineBindPoint bindPoint )
	{
		uint32_t frame = Renderer::Get()->GetCurrentFrame();
		Update( rExtraWds );

		VkDescriptorSet Set = m_DescriptorSets[ frame ];
		vkCmdBindDescriptorSets( CommandBuffer, bindPoint, Layout, 0, 1, &Set, 0, nullptr );
	}

	void Material::Update( const std::vector<std::vector<VkWriteDescriptorSet>>& rExtraWds )
	{
		uint32_t frame = Renderer::Get()->GetCurrentFrame();

		if( rExtraWds.size() )
		{
			for( const auto& rWds : rExtraWds[ frame ] )
			{
				PushExternalWds( rWds );
			}
		}

		RT_Update();
	}

	void Material::RT_Update()
	{
		SAT_PF_EVENT();

		// This material has no descriptor sets
		if( m_DescriptorSetTemplate.Set == UINT32_MAX ) return;

		uint32_t frame = Renderer::Get()->GetCurrentFrame();

		m_DescriptorSets[ frame ] = m_Shader->AllocateDescriptorSet( m_Set, true );

		const size_t reservedSize = m_DescriptorSetTemplate.SampledImages.size() + m_DescriptorSetTemplate.StorageImages.size() + m_DescriptorSetTemplate.UniformBuffers.size() + m_DescriptorSetTemplate.StorageBuffers.size();

		std::vector<VkWriteDescriptorSet> pendingWds;
		pendingWds.reserve( reservedSize );

		// Sampled images
		for( auto& texture : m_DescriptorSetTemplate.SampledImages )
		{
			// Texture arrays will be handled differently.
			// TODO: Handle texture arrays here
			if( texture.ArraySize > 1 )
				continue;

			m_DescriptorSetTemplate.WriteDescriptorSets[ texture.Binding ].dstSet = m_DescriptorSets[ frame ];

			pendingWds.push_back( m_DescriptorSetTemplate.WriteDescriptorSets[ texture.Binding ] );
		}

		// Storage images
		for( auto& texture : m_DescriptorSetTemplate.StorageImages )
		{
			m_DescriptorSetTemplate.WriteDescriptorSets[ texture.Binding ].dstSet = m_DescriptorSets[ frame ];

			pendingWds.push_back( m_DescriptorSetTemplate.WriteDescriptorSets[ texture.Binding ] );
		}

		// Uniform buffers
		for( auto& [binding, ub] : m_DescriptorSetTemplate.UniformBuffers )
		{
			m_DescriptorSetTemplate.WriteDescriptorSets[ binding ].dstSet = m_DescriptorSets[ frame ];

			pendingWds.push_back( m_DescriptorSetTemplate.WriteDescriptorSets[ binding ] );
		}

		// SB buffers
		for( auto& [binding, sb] : m_DescriptorSetTemplate.StorageBuffers )
		{
			m_DescriptorSetTemplate.WriteDescriptorSets[ binding ].dstSet = m_DescriptorSets[ frame ];
		
			pendingWds.push_back( m_DescriptorSetTemplate.WriteDescriptorSets[ binding ] );
		}

		// Texture Arrays
		for( auto& [name, textures] : m_TextureArrays )
		{
			std::vector<VkDescriptorImageInfo> ImageInfos( textures.size() );

			size_t index = 0;
			for( auto& texture : textures )
			{
				ImageInfos[ index ] = texture->GetDescriptorInfo();
				++index;
			}

			const auto Itr = std::find_if( m_DescriptorSetTemplate.SampledImages.begin(), m_DescriptorSetTemplate.SampledImages.end(),
				[ name ]( const ShaderSampledImage& rImage )
			{
				return rImage.Name == name;
			} );

			if( Itr != m_DescriptorSetTemplate.SampledImages.end() )
			{
				auto& rWds = m_DescriptorSetTemplate.WriteDescriptorSets[ Itr->Binding ];
				rWds.pImageInfo = ImageInfos.data();
				rWds.dstSet = m_DescriptorSets[ frame ];
				rWds.descriptorCount = ( uint32_t ) ImageInfos.size();

				vkUpdateDescriptorSets( VulkanContext::Get()->GetDevice(), 1, &rWds, 0, nullptr );
			}
		}

		if( pendingWds.size() )
		{
			vkUpdateDescriptorSets( VulkanContext::Get()->GetDevice(), (uint32_t)pendingWds.size(), pendingWds.data(), 0, nullptr );
		}
	}

	void Material::PushExternalWds( const VkWriteDescriptorSet& rWds )
	{
		m_DescriptorSetTemplate.WriteDescriptorSets[ rWds.dstBinding ] = rWds;
	}

	void Material::SetResource( const std::string& Name, const Ref<Texture2D>& Texture )
	{
		m_Textures[ Name ] = Texture;

		auto Itr = std::find_if( m_DescriptorSetTemplate.SampledImages.begin(), m_DescriptorSetTemplate.SampledImages.end(),
			[ Name ]( const ShaderSampledImage& rImage )
		{
			return rImage.Name == Name;
		} );

		if( Itr != m_DescriptorSetTemplate.SampledImages.end() )
		{
			ShaderSampledImage ssi = *( Itr );
			m_DescriptorSetTemplate.WriteDescriptorSets[ ssi.Binding ].pImageInfo = &Texture->GetDescriptorInfo();
		}
		else
		{
			// Check for if resource is a storage image
			Itr = std::find_if( m_DescriptorSetTemplate.StorageImages.begin(), m_DescriptorSetTemplate.StorageImages.end(),
				[ Name ]( const ShaderSampledImage& rImage )
			{
				return rImage.Name == Name;
			} );

			if( Itr != m_DescriptorSetTemplate.StorageImages.end() )
			{
				ShaderSampledImage ssi = *( Itr );
				m_DescriptorSetTemplate.WriteDescriptorSets[ ssi.Binding ].pImageInfo = &Texture->GetDescriptorInfo();
			}
		}
	}

	void Material::SetResource( const std::string& Name, const Ref<Texture2D>& Texture, uint32_t Index )
	{
		auto& textures = m_TextureArrays[ Name ];

		if( textures.size() >= Index )
			textures.resize( Index + 1 );

		textures[ Index ] = Texture;
	}

	void Material::SetResource( const std::string& Name, Ref<Image2D> rImage )
	{
		auto Itr = std::find_if( m_DescriptorSetTemplate.SampledImages.begin(), m_DescriptorSetTemplate.SampledImages.end(),
			[ Name ]( const ShaderSampledImage& rImage )
		{
			return rImage.Name == Name;
		} );

		if( Itr != m_DescriptorSetTemplate.SampledImages.end() )
		{
			ShaderSampledImage ssi = *( Itr );
			m_DescriptorSetTemplate.WriteDescriptorSets[ ssi.Binding ].pImageInfo = &rImage->GetDescriptorInfo();
		}
		else
		{
			// Check for if resource is a storage image
			Itr = std::find_if( m_DescriptorSetTemplate.StorageImages.begin(), m_DescriptorSetTemplate.StorageImages.end(),
				[ Name ]( const ShaderSampledImage& rImage )
			{
				return rImage.Name == Name;
			} );

			if( Itr != m_DescriptorSetTemplate.StorageImages.end() )
			{
				ShaderSampledImage ssi = *( Itr );
				m_DescriptorSetTemplate.WriteDescriptorSets[ ssi.Binding ].pImageInfo = &rImage->GetDescriptorInfo();
			}
		}
	}

	void Material::SetResource( const std::string& Name, const Ref<TextureCube>& Texture )
	{
		auto Itr = std::find_if( m_DescriptorSetTemplate.SampledImages.begin(), m_DescriptorSetTemplate.SampledImages.end(),
			[ Name ]( const ShaderSampledImage& rImage )
		{
			return rImage.Name == Name;
		} );

		if( Itr != m_DescriptorSetTemplate.SampledImages.end() )
		{
			ShaderSampledImage ssi = *( Itr );
			m_DescriptorSetTemplate.WriteDescriptorSets[ ssi.Binding ].pImageInfo = &Texture->GetDescriptorInfo();
		}
		else
		{
			// Check for if resource is a storage image
			Itr = std::find_if( m_DescriptorSetTemplate.StorageImages.begin(), m_DescriptorSetTemplate.StorageImages.end(),
				[ Name ]( const ShaderSampledImage& rImage )
			{
				return rImage.Name == Name;
			} );

			if( Itr != m_DescriptorSetTemplate.StorageImages.end() )
			{
				ShaderSampledImage ssi = *( Itr );
				m_DescriptorSetTemplate.WriteDescriptorSets[ ssi.Binding ].pImageInfo = &Texture->GetDescriptorInfo();
			}
		}
	}

	Ref<Texture2D> Material::GetResource( const std::string& Name )
	{
		if( m_Textures.size() > 0 )
			return m_Textures.at( Name );
		else
			return nullptr;
	}

	Ref<UniformBuffer> Material::GetOrCreateUB( uint32_t binding )
	{
		SAT_PF_EVENT();

		uint32_t frame = Renderer::Get()->GetCurrentFrame();

		auto& ubs = m_UniformBuffers[ binding ];

		if( ubs[ frame ] == nullptr )
		{
			ubs[ frame ] = Ref<UniformBuffer>::Create( m_Set, binding, m_DescriptorSetTemplate.UniformBuffers[ binding ].Size );

			m_DescriptorSetTemplate.WriteDescriptorSets[ binding ].pBufferInfo = &ubs[ frame ]->GetBufferInfo();
		}

		return ubs[ frame ];
	}

	void Material::UploadDataToUB( uint32_t Binding, void* pData, size_t size )
	{
		Ref<UniformBuffer> ub = GetOrCreateUB( Binding );
		ub->UploadData( pData, size, 0 );
	}

	void Material::SetSB( uint32_t binding, const Ref<StorageBuffer>& rSB )
	{
		m_DescriptorSetTemplate.WriteDescriptorSets[ binding ].pBufferInfo = &rSB->GetBufferInfo();
	}

}
