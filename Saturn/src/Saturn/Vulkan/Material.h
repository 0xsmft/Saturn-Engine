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

#pragma once

#include "Texture.h"
#include "Shader.h"

#include "UniformBuffer.h"
#include "StorageBuffer.h"

#include <string>

namespace Saturn {

	class Material : public RefTarget
	{
	public:
		 Material( const Ref<Shader>& rShader, const std::string& rMateralName, uint32_t set = 0 );
		~Material();

		void Initialise( const std::string& rMaterialName );
		void Copy( Ref<Material>& rOther );
		void SetName( const std::string& rName ) { m_Name = rName; }

		// Bind and update the material descriptor set.
		void Bind( VkCommandBuffer CommandBuffer, VkPipelineLayout Layout, const std::vector<std::vector<VkWriteDescriptorSet>>& rExtraWds, VkPipelineBindPoint bindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS );

		void Update( const std::vector<std::vector<VkWriteDescriptorSet>>& rExtraWds );

		void SetResource( const std::string& Name, const Ref<Texture2D>& Texture );
		void SetResource( const std::string& Name, const Ref<Texture2D>& Texture, uint32_t Index );
		void SetResource( const std::string& Name, const Ref<TextureCube>& Texture );
		void SetResource( const std::string& Name, Ref<Image2D> rImage );

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
		
		VkDescriptorSet GetDescriptorSet(uint32_t index = 0) const { return m_DescriptorSets[index]; }

		void UploadDataToUB( uint32_t Binding, void* pData, size_t size );
		void SetSB( uint32_t binding, const Ref<StorageBuffer>& rInfo );

	public:
		Ref<Shader> GetShader() { return m_Shader; }
		
		std::string& GetName() { return m_Name; }
		const std::string& GetName() const { return m_Name; }

		Buffer GetPushConstantData() const { return m_PushConstantData; }

	private:
		std::unordered_map< std::string, Ref<Texture2D> >& GetTextures() { return m_Textures; }
		const std::unordered_map< std::string, Ref<Texture2D> >& GetTextures() const { return m_Textures; }

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

		VkDescriptorSet m_DescriptorSets[ MAX_FRAMES_IN_FLIGHT ];

	private:
		friend class MaterialAsset;
	};
}
