/********************************************************************************************
*                                                                                           *
*                                                                                           *
*                                                                                           *
* MIT License                                                                               *
*                                                                                           *
* Copyright (c) 2020 - 2025 BEAST                                                           *
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

// TODO: When we have an asset manager, this needs to be re-worked!

namespace Saturn {

	Material::Material( const Ref< Saturn::Shader >& Shader, const std::string& MateralName )
		: m_Shader( Shader )
	{
		Initialise( MateralName );
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
		ShaderDescriptorSetTemplate& rMaterialDS = m_Shader->GetShaderDescriptorSetTemplates( 0 );
		
		// Copy for our own use
		m_DescriptorSetTemplate = ShaderDescriptorSetTemplate( rMaterialDS );

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
	}

	void Material::Bind( VkCommandBuffer CommandBuffer, VkPipelineLayout Layout, const std::vector<VkWriteDescriptorSet>& rExtraWds, VkPipelineBindPoint bindPoint )
	{
		for( const auto& rWds : rExtraWds )
		{
			PushExternalWds( rWds );
		}

		uint32_t frame = Renderer::Get().GetCurrentFrame();
		RT_Update();

		VkDescriptorSet Set = m_DescriptorSets[ frame ];
		vkCmdBindDescriptorSets( CommandBuffer, bindPoint, Layout, 0, 1, &Set, 0, nullptr );
	}

	void Material::RT_Update()
	{
		uint32_t frame = Renderer::Get().GetCurrentFrame();

		m_DescriptorSets[ frame ] = m_Shader->AllocateDescriptorSet( 0, true );

		std::vector<VkWriteDescriptorSet> pendingWds;

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
			std::vector<VkDescriptorImageInfo> ImageInfos;

			for( auto& texture : textures )
			{
				ImageInfos.push_back( texture->GetDescriptorInfo() );
			}

			auto Itr = std::find_if( m_DescriptorSetTemplate.SampledImages.begin(), m_DescriptorSetTemplate.SampledImages.end(),
				[ name ]( const ShaderSampledImage& rImage )
			{
				return rImage.Name == name;
			} );

			if( Itr != m_DescriptorSetTemplate.SampledImages.end() )
			{
				auto& rWds = m_DescriptorSetTemplate.WriteDescriptorSets[ Itr->Binding ];
				rWds.pImageInfo = ImageInfos.data();
				rWds.descriptorCount = ImageInfos.size();
				rWds.dstSet = m_DescriptorSets[ frame ];

				vkUpdateDescriptorSets( VulkanContext::Get().GetDevice(), 1, &rWds, 0, nullptr );
			}
		}

		if( pendingWds.size() )
		{
			vkUpdateDescriptorSets( VulkanContext::Get().GetDevice(), (uint32_t)pendingWds.size(), pendingWds.data(), 0, nullptr );
		}
	}

	void Material::PushExternalWds( const VkWriteDescriptorSet& rWds )
	{
		if( rWds.descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER || rWds.descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER )
		{
			m_DescriptorSetTemplate.WriteDescriptorSets[ rWds.dstBinding ] = rWds;
		}

		if( rWds.descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE || rWds.descriptorType == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE )
		{
			m_DescriptorSetTemplate.WriteDescriptorSets[ rWds.dstBinding ] = rWds;
		}
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
				ShaderSampledImage ssi;
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
		uint32_t frame = Renderer::Get().GetCurrentFrame();

		std::vector<Ref<UniformBuffer>> ubs = m_UniformBuffers[ binding ];

		auto Itr = std::find( ubs.begin(), ubs.end(), frame + 1 );

		if( Itr == ubs.end() )
		{
			auto ub = Ref<UniformBuffer>::Create( 0, binding, m_DescriptorSetTemplate.UniformBuffers[ binding ].Size );

			m_UniformBuffers[ binding ].push_back( ub );

			m_DescriptorSetTemplate.WriteDescriptorSets[ binding ].pBufferInfo = &ub->GetBufferInfo();

			return ub;
		}

		return m_UniformBuffers[ binding ][ frame ];
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