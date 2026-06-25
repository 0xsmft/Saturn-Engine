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

#include "Texture.h"
#include "Shader.h"
#include "Sampler.h"

#include "UniformBuffer.h"
#include "StorageBuffer.h"

#include <string>

namespace Saturn {

	//
	// Material
	// 
	// A material represents a certain descriptor set in it's shader
	// By default it represents set 0.
	// 
	// A material must be updated and the bound before it can be used in a draw call.
	// 
	// Materials can be used in any type of pipeline that we support i.e. Graphics or Compute.
	// 
	// Materials can be used in any place where we need to supply our shader some information.
	// For meshes (both Static and Skeletal) MaterialAssets are used and they ONLY represent set 0.
	//
	class Material : public RefTarget
	{
	public:
		 Material( const Ref<Shader>& rShader, const std::string& rMaterialName, uint32_t set = 0 );
		virtual ~Material();

		void Copy( Ref<Material>& rOther );
		void SetName( const std::string& rName ) { m_Name = rName; }

	public:
		// Bind and update the material descriptor set.
		//
		// Calls vkCmdBindDescriptorSets
		//
		void Bind( VkCommandBuffer CommandBuffer, VkPipelineLayout Layout, const std::vector<std::vector<VkWriteDescriptorSet>>& rExtraWds, VkPipelineBindPoint bindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS );

		// Update the descriptor set.
		void Update( const std::vector<std::vector<VkWriteDescriptorSet>>& rExtraWds );

		// 
		// Set a single texture resource.
		// 
		// @param Name -- the name of the resource in the SHADER e.g. u_MyTexture
		//
		void SetResource( const std::string& Name, const Ref<Texture2D>& Texture );

		// 
		// Set a image view resource.
		// 
		// NB: The texture param MUST be the same as the
		// 
		// @param Name -- the name of the resource in the SHADER e.g. u_MyTexture
		// @param Texture -- the texture
		// @param ImageViewTexture -- image view index that the image view will come from
		//
		void SetResourceWithVulkanInfo( const std::string& Name, Ref<Texture2D> Texture, const VkDescriptorImageInfo& rVulkanInfo );

		// 
		// Set a single texture resource in a texture array.
		// 
		// @param Name -- the name of the resource in the SHADER e.g. u_MyTextureArray
		//
		void SetResource( const std::string& Name, const Ref<Texture2D>& Texture, uint32_t Index );

		// 
		// Set a single texture cube resource in a texture.
		// 
		// @param Name -- the name of the resource in the SHADER e.g. u_MyTextureCube
		//
		void SetResource( const std::string& Name, const Ref<TextureCube>& Texture );

		// 
		// Set a Image2D resource.
		// 
		// NOTE: Not used by MaterialAssets! But used by other shader materials
		// 
		// @param Name -- the name of the resource in the SHADER e.g. u_MyTexture
		//
		void SetResource( const std::string& Name, Ref<Image2D> rImage );

		// 
		// Set a Sampler resource.
		// 
		// NOTE: Not used by MaterialAssets! But used by other shader materials
		// 
		// @param Name -- the name of the resource in the SHADER e.g. s_MySampler
		//
		void SetResource( const std::string& Name, const Ref<Sampler> sampler );

		//
		// Set an image i.e. (VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE) with no sampler
		// 
		// NOTE: Not used by MaterialAssets! But used by other shader materials
		//
		// @param Name -- the name of the resource in the SHADER e.g. s_MySampler
		//
		void SetSeparateImage( const std::string& Name, const Ref<Image2D> image );

		// 
		// Get texture2D resource.
		// 
		// NB: Will return null if not found!
		// 
		// @param Name -- the name of the resource in the SHADER e.g. u_MyTexture
		//
		Ref<Texture2D> GetResource( const std::string& Name );

		// Set push constant data.
		template<typename Ty>
		void SetPC( const std::string& Name, const Ty& Value ) 
		{
			uint32_t offset = m_ShaderPC.MemberOffsets[ Name ];
			m_PushConstantData.Write( ( uint8_t* ) &Value, sizeof( Ty ), offset );
		}
		
		// Get push constant data.
		template<typename Ty>
		Ty& Get( const std::string& Name ) 
		{
			uint32_t offset = m_ShaderPC.MemberOffsets[ Name ];
			return m_PushConstantData.Read<Ty>( offset );
		}
		
		VkDescriptorSet GetDescriptorSet( uint32_t index = 0 ) const { return m_DescriptorSets[ index ]; }

		//
		// Upload data to a uniform buffer
		//
		void UploadDataToUB( uint32_t Binding, void* pData, size_t size );

		//
		// Set the storage buffer
		//
		void SetSB( uint32_t binding, const Ref<StorageBuffer>& rInfo );

	public:
		Ref<Shader> GetShader() { return m_Shader; }
		
		std::string& GetName() { return m_Name; }
		const std::string& GetName() const { return m_Name; }

		Buffer GetPushConstantData() const { return m_PushConstantData; }

	private:
		std::unordered_map< std::string, Ref<Texture2D> >& GetTexturesMap() { return m_Textures; }
		const std::unordered_map< std::string, Ref<Texture2D> >& GetTexturesMap() const { return m_Textures; }

		void Initialise( const std::string& rMaterialName );

		// Internal Update function without binding the descriptor set.
		void RT_Update();
		void InitLayout();
		void PushExternalWds( const VkWriteDescriptorSet& rWds );
		Ref<UniformBuffer> GetOrCreateUB( uint32_t binding );

	private:
		uint32_t m_Set = 0;
		std::string m_Name = "";
		Ref<Shader> m_Shader;

		Buffer m_PushConstantData;
		
		// Binding -> UniformBuffers (per frame in flight)
		std::unordered_map< uint32_t, std::array< Ref<UniformBuffer>, MAX_FRAMES_IN_FLIGHT > > m_UniformBuffers;

		// Binding Name -> Textures
		std::unordered_map< std::string, Ref<Texture2D> > m_Textures;
		std::unordered_map< std::string, std::vector< Ref<Texture2D> > > m_TextureArrays;

		// DescriptorSetTemplate, PushConstantTemplate
		ShaderDescriptorSetTemplate m_DescriptorSetTemplate;
		ShaderPushConstantTemplate m_ShaderPC{};

		std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT> m_DescriptorSets;

	private:
		friend class MaterialAsset;
	};
}
