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

		for( auto&& texture : m_Shader->GetTextures() )
		{
			m_Textures[ texture.Name ] = nullptr;
		}

		// Intentional copy of shader uniforms.
		m_Uniforms.reserve( m_Shader->GetUniforms().size() );

		for( auto& rUniform : m_Shader->GetUniforms() )
		{
			m_Uniforms.push_back( { rUniform.Name, rUniform.Location, rUniform.DataType, rUniform.Size, rUniform.Offset, rUniform.IsPushConstantData } );
		}

		uint32_t Size = 0;

		for( auto& rUniform : m_Uniforms )
		{
			if( rUniform.IsPushConstantData )
			{
				Size += static_cast< uint32_t >( rUniform.Size );
			}
		}

		m_PushConstantData.Allocate( Size );
		m_PushConstantData.Zero_Memory();
	}

	Material::~Material()
	{
		for ( auto& uniform : m_Uniforms )
			uniform.Terminate();

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
		m_Uniforms.clear();

		m_Textures = rOther->m_Textures;
		m_Uniforms = rOther->m_Uniforms;

		m_Name = rOther->GetName();
		m_AnyValueChanged = rOther->HasAnyValueChanged();

		m_Shader = rOther->m_Shader;
		m_PushConstantData = rOther->m_PushConstantData;
	}

	void Material::Bind( VkCommandBuffer CommandBuffer, Ref<Shader>& rShader )
	{
		uint32_t frame = Renderer::Get().GetCurrentFrame();

		RT_Update();

		VkDescriptorSet Set = m_DescriptorSets[ frame ];
		rShader->WriteAllUBs( Set );
	}

	void Material::BindDS( VkCommandBuffer CommandBuffer, VkPipelineLayout Layout )
	{
		uint32_t frame = Renderer::Get().GetCurrentFrame();
		VkDescriptorSet Set = m_DescriptorSets[ frame ];

		vkCmdBindDescriptorSets( CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, Layout, 0, 1, &Set, 0, nullptr );
	}

	void Material::RT_Update()
	{
		uint32_t frame = Renderer::Get().GetCurrentFrame();

		m_DescriptorSets[ frame ] = m_Shader->AllocateDescriptorSet( 0, true );

		for( auto& [name, texture] : m_Textures )
		{
			VkDescriptorImageInfo ImageInfo = {};
			ImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

			ImageInfo.imageView = m_Textures[ name ]->GetImageView();
			ImageInfo.sampler = m_Textures[ name ]->GetSampler();

			m_Shader->WriteDescriptor( name, ImageInfo, m_DescriptorSets[ frame ] );
		}

		for( auto& [name, textures] : m_TextureArrays )
		{
			std::vector<VkDescriptorImageInfo> ImageInfos;

			for ( auto& texture : textures )
			{
				ImageInfos.push_back( texture->GetDescriptorInfo() );
			}

			m_Shader->WriteDescriptor( name, ImageInfos, m_DescriptorSets[ frame ] );
		}

		m_Shader->WriteAllUBs( m_DescriptorSets[ frame ] );
	}

	void Material::SetResource( const std::string& Name, const Ref<Texture2D>& Texture )
	{
		if( m_Textures[ Name ] )
			m_AnyValueChanged = true;

		m_Textures[ Name ] = Texture;
	}

	void Material::SetResource( const std::string& Name, const Ref<Texture2D>& Texture, uint32_t Index )
	{
		auto& textures = m_TextureArrays[ Name ];

		if( textures.size() >= Index )
			textures.resize( Index + 1 );

		textures[ Index ] = Texture;
	}

	Ref<Texture2D> Material::GetResource( const std::string& Name )
	{
		if( m_Textures.size() > 0 )
			return m_Textures.at( Name );
		else
			return nullptr;
	}

	void Material::WriteDescriptor( VkWriteDescriptorSet& rWDS )
	{
		uint32_t frame = Renderer::Get().GetCurrentFrame();
		rWDS.dstSet = m_DescriptorSets[ frame ];

		vkUpdateDescriptorSets( VulkanContext::Get().GetDevice(), 1, &rWDS, 0, nullptr );
	}
}