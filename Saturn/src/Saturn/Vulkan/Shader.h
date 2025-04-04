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

#include "Saturn/Core/UUID.h"

#include "DescriptorSet.h"

#include "Base.h"
#include "Saturn/Serialisation/RawSerialisation.h"

#include <vector>
#include <string>
#include <filesystem>
#include <vulkan.h>
#include <unordered_map>

namespace Saturn {
	
	enum class ShaderType : uint32_t
	{
		None = 0,
		Vertex = 1,
		Fragment = 2,
		Geometry = 3,
		Compute = 4,
		All = 5
	};
	
	// ShaderUniformBuffer
	// Consider this class as a description to a uniform buffer
	// For example in a GLSL shader if we have:
	// 
	// layout(set = 0, binding = 0) uniform Matrices 
	// {
	//   mat4 ViewProjection;
	// 	 mat4 View;
	// } u_Matrices;
	// Our C++ information would be 
	// u_Matrices, 0, 128, ShaderType::Vertex
	// NOTE: ShaderUniformBuffer do not allocate a Vulkan Buffer. That is done via the Uniform Buffer class
	struct ShaderUniformBuffer
	{
		std::string Name;
		uint32_t Binding;
		size_t Size = 0;
		ShaderType Location = ShaderType::None;

		bool operator==( const ShaderUniformBuffer& rOther ) 
		{
			return Binding == rOther.Binding && Name == rOther.Name && Size == rOther.Size;
		}

		auto operator<=>( const ShaderUniformBuffer& rOther ) const = default;

		static void Serialise( const ShaderUniformBuffer& rObject, std::ofstream& rStream )
		{
			RawSerialisation::WriteString( rObject.Name, rStream );
			RawSerialisation::WriteObject( rObject.Binding, rStream );
			RawSerialisation::WriteObject( rObject.Size, rStream );
			RawSerialisation::WriteObject( rObject.Location, rStream );
		}

		static void Deserialise( ShaderUniformBuffer& rObject, std::ifstream& rStream )
		{
			rObject.Name = RawSerialisation::ReadString( rStream );
			RawSerialisation::ReadObject( rObject.Binding, rStream );
			RawSerialisation::ReadObject( rObject.Size, rStream );
			RawSerialisation::ReadObject( rObject.Location, rStream );
		}
	};

	// ShaderStorageBuffer
	// Consider this class as a description to a storage buffer
	// For example in a GLSL shader if we have:
	//
	// layout( std430, set = 0, binding = 14 ) buffer VisiblePointLightIndicesBuffer
	// {
	//	 int Indices[];
	// } s_VisiblePointLightIndicesBuffer;
	// 
	// Our C++ information would be:
	// s_VisiblePointLightIndicesBuffer, 14, -, ShaderType::Fragment/Compute
	// NOTE: ShaderStorageBuffer do not allocate a Vulkan Buffer. That is done via the Storage Buffer class
	struct ShaderStorageBuffer
	{
		std::string Name;
		uint32_t Binding;
		size_t Size = 0;
		ShaderType Location = ShaderType::None;

		bool operator==( const ShaderStorageBuffer& rOther )
		{
			return Binding == rOther.Binding && Name == rOther.Name && Size == rOther.Size;
		}

		auto operator<=>( const ShaderStorageBuffer& rOther ) const = default;

		static void Serialise( const ShaderStorageBuffer& rObject, std::ofstream& rStream )
		{
			RawSerialisation::WriteString( rObject.Name, rStream );
			RawSerialisation::WriteObject( rObject.Binding, rStream );
			RawSerialisation::WriteObject( rObject.Size, rStream );
			RawSerialisation::WriteObject( rObject.Location, rStream );
		}

		static void Deserialise( ShaderStorageBuffer& rObject, std::ifstream& rStream )
		{
			rObject.Name = RawSerialisation::ReadString( rStream );
			RawSerialisation::ReadObject( rObject.Binding, rStream );
			RawSerialisation::ReadObject( rObject.Size, rStream );
			RawSerialisation::ReadObject( rObject.Location, rStream );
		}
	};

	// ShaderSampledImage
	// Consider this class as a description to a sampled image
	// For example in a GLSL shader if we have:
	//
	// layout(set = 0, binding = 1) uniform sampler2D u_AlbedoTexture;
	// Our C++ information would be:
	// u_AlbedoTexture, ShaderType::Fragment, 0, 1, 1
	struct ShaderSampledImage
	{
		std::string Name;
		ShaderType Stage = ShaderType::None;
		uint32_t Set;
		uint32_t Binding;
		uint32_t ArraySize;

		static void Serialise( const ShaderSampledImage& rObject, std::ofstream& rStream )
		{
			RawSerialisation::WriteString( rObject.Name, rStream );
			RawSerialisation::WriteObject( rObject.Stage, rStream );
			RawSerialisation::WriteObject( rObject.Set, rStream );
			RawSerialisation::WriteObject( rObject.Binding, rStream );
			RawSerialisation::WriteObject( rObject.ArraySize, rStream );
		}

		static void Deserialise( ShaderSampledImage& rObject, std::ifstream& rStream )
		{
			rObject.Name = RawSerialisation::ReadString( rStream );

			RawSerialisation::ReadObject( rObject.Stage, rStream );
			RawSerialisation::ReadObject( rObject.Set, rStream );
			RawSerialisation::ReadObject( rObject.Binding, rStream );
			RawSerialisation::ReadObject( rObject.ArraySize, rStream );
		}
	};

	// ShaderPushConstantTemplate
	// Consider this class as a description to a push constant buffer
	// For example in a GLSL shader if we have:
	//
	// layout(push_constant) uniform PushConstantData
	// {
	//   mat4 ViewProjection;
	//   mat4 View;
	// } u_PushConstantData;
	//
	// Our C++ information would be:
	// u_PushConstantData, ShaderType::Vertex, 128, MemberOffsets[(u_PushConstantData.ViewProjection, 0), (u_PushConstantData.View, 64)]
	struct ShaderPushConstantTemplate
	{
		std::string Name;
		ShaderType Stage = ShaderType::None;
		uint32_t Size = 0;

		// NAME -> OFFSET
		std::unordered_map<std::string, uint32_t> MemberOffsets;

	public:
		static void Serialise( const ShaderPushConstantTemplate& rPushConst, std::ofstream& rStream )
		{
			RawSerialisation::WriteString( rPushConst.Name, rStream );
			RawSerialisation::WriteObject( rPushConst.Stage, rStream );
			RawSerialisation::WriteObject( rPushConst.Size, rStream );
			RawSerialisation::WriteUnorderedMap( rPushConst.MemberOffsets, rStream );
		}

		static void Deserialise( ShaderPushConstantTemplate& rPushConst, std::ifstream& rStream )
		{
			rPushConst.Name = RawSerialisation::ReadString( rStream );
			RawSerialisation::ReadObject( rPushConst.Stage, rStream );
			RawSerialisation::ReadObject( rPushConst.Size, rStream );
			RawSerialisation::ReadUnorderedMap( rPushConst.MemberOffsets, rStream );
		}
	};

	// ShaderDescriptorSetTemplate
	// Consider this class as a template/layout/description to a descriptor set
	// For example in a GLSL shader if we have:
	// 
	// layout(set = 0, binding = 0) uniform Matrices 
	// {
	//   mat4 ViewProjection;
	// 	 mat4 View;
	// } u_Matrices;
	// 
	// layout (set = 0, binding = 1) uniform sampler2D u_AlbedoTexture;
	// 
	// layout( std430, set = 0, binding = 14 ) buffer VisiblePointLightIndicesBuffer
	// {
	//	 int Indices[];
	// } s_VisiblePointLightIndicesBuffer;
	// 
	// Our C++ information would be:
	// Set = 0
	// SampledImages = 1 (u_AlbedoTexture [ShaderSampledImage])
	// UniformBuffers = 1 (u_Matrices [ShaderUniformBuffer])
	// StorageBuffers = 1 (s_VisiblePointLightIndicesBuffer [ShaderStorageBuffer])
	// WriteDescriptorSets = 2 (u_AlbedoTexture image wds, u_Matrices texture wds, s_VisiblePointLightIndicesBuffer wds )
	// 
	// ShaderDescriptorSetTemplate do not own or create a Vulkan DescriptorSet it is simply used for information about the descriptor set. 
	// To allocate a descriptor set with such information you'd need to use the shader to create it with the correct set.
	// ShaderDescriptorSetTemplate does contain the Vulkan Descriptor Set Layout
	class ShaderDescriptorSetTemplate
	{
	public:
		ShaderDescriptorSetTemplate() = default;
		ShaderDescriptorSetTemplate( uint32_t set ) : Set( set ) {}

		ShaderDescriptorSetTemplate( const ShaderDescriptorSetTemplate& rOther )
		{
			Set = rOther.Set;
			SetLayout = rOther.SetLayout;

			WriteDescriptorSets = rOther.WriteDescriptorSets;
			SampledImages       = rOther.SampledImages;
			StorageImages       = rOther.StorageImages;
			UniformBuffers      = rOther.UniformBuffers;
			StorageBuffers      = rOther.StorageBuffers;
		}

		~ShaderDescriptorSetTemplate() = default;

	public:
		uint32_t Set = UINT32_MAX;

		VkDescriptorSetLayout SetLayout = nullptr;
		
		// BINDING -> WDS
		std::unordered_map< uint32_t, VkWriteDescriptorSet > WriteDescriptorSets;
		std::unordered_map< uint32_t, ShaderUniformBuffer > UniformBuffers;
		std::unordered_map< uint32_t, ShaderStorageBuffer > StorageBuffers;

		std::vector< ShaderSampledImage > SampledImages;
		std::vector< ShaderSampledImage > StorageImages;

	public:
		static void Serialise( const ShaderDescriptorSetTemplate& rObject, std::ofstream& rStream )
		{
			RawSerialisation::WriteObject( rObject.Set, rStream );

			RawSerialisation::WriteVector( rObject.SampledImages, rStream );
			RawSerialisation::WriteVector( rObject.StorageImages, rStream );

			RawSerialisation::WriteUnorderedMap( rObject.UniformBuffers, rStream );
			RawSerialisation::WriteUnorderedMap( rObject.StorageBuffers, rStream );
		}

		static void Deserialise( ShaderDescriptorSetTemplate& rObject, std::ifstream& rStream )
		{
			RawSerialisation::ReadObject( rObject.Set, rStream );

			RawSerialisation::ReadVector( rObject.SampledImages, rStream );
			RawSerialisation::ReadVector( rObject.StorageImages, rStream );

			RawSerialisation::ReadUnorderedMap( rObject.UniformBuffers, rStream );
			RawSerialisation::ReadUnorderedMap( rObject.StorageBuffers, rStream );
		}
	};

	struct ShaderSource
	{
		ShaderSource() {}
		~ShaderSource() {}
		
		ShaderSource( const std::string& rSrc, ShaderType Type, int Index )
			: Source( rSrc ), Type( Type ), Index( Index )
		{
		}

		std::string Source = "";
		ShaderType Type = ShaderType::Vertex;
		int Index = -1;
	};

	struct ShaderSourceKey
	{
		ShaderSourceKey() {}
		ShaderSourceKey( ShaderType _Type, int _Index ) : Type( _Type ), Index( _Index ) {}
		~ShaderSourceKey() {}

		ShaderSourceKey operator=( const ShaderSourceKey& rKey )
		{
			Type = rKey.Type;
			Index = rKey.Index;
			return *this;
		}
		
		bool operator==( const ShaderSourceKey& rKey )
		{
			return ( Type == rKey.Type && Index == rKey.Index );
		}
		
		bool operator==( const ShaderSourceKey& rKey ) const
		{
			return ( Type == rKey.Type && Index == rKey.Index );
		}

		ShaderType Type = ShaderType::Vertex;
		int Index = -1;

		static void Serialise( const ShaderSourceKey& rKey, std::ofstream& rStream )
		{
			RawSerialisation::WriteObject( rKey.Type, rStream );
			RawSerialisation::WriteObject( rKey.Index, rStream );
		}

		static void Deserialise( ShaderSourceKey& rKey, std::ifstream& rStream )
		{
			RawSerialisation::ReadObject( rKey.Type, rStream );
			RawSerialisation::ReadObject( rKey.Index, rStream );
		}
	};
}

namespace std {

	template<>
	struct hash< Saturn::ShaderSourceKey >
	{
		size_t operator()( const Saturn::ShaderSourceKey& rKey ) const
		{
			return ( size_t ) rKey.Index;
		}
	};

	template<>
	struct hash< Saturn::ShaderSource >
	{
		size_t operator()( const Saturn::ShaderSource& rKey )
		{
			return hash< std::string >()( rKey.Source ) ^ ( ( size_t )rKey.Type << 32 );
		}
	};

}

namespace Saturn {

	class Shader : public RefTarget
	{
		using ShaderSourceMap = std::unordered_map< ShaderSourceKey, ShaderSource >;
		using SpvSourceMap = std::unordered_map< ShaderSourceKey, std::vector< uint32_t > >;
	public:
		// Internal default constructor, only used when reading from a shader bundle.
		// Do not use!
		Shader() {}
	
		Shader( const std::filesystem::path& rFilepath );
		~Shader();
	
		std::string& GetName() { return m_Name; }
		const std::string& GetName() const { return m_Name; }

#if !defined(SAT_DIST)
		ShaderSourceMap& GetShaderSources() { return m_ShaderSources; }
		const ShaderSourceMap& GetShaderSources() const { return m_ShaderSources; }
#endif

		const SpvSourceMap& GetSpvCode() const { return m_SpvCode; }
		SpvSourceMap& GetSpvCode() { return m_SpvCode; }
		
		void WriteDescriptor( const std::string& rName, VkDescriptorImageInfo& rImageInfo, VkDescriptorSet desSet );

		Ref< DescriptorPool >& GetDescriptorPool() { return m_SetPool; }
		const Ref< DescriptorPool >& GetDescriptorPool() const { return m_SetPool; }
		
		std::vector< ShaderPushConstantTemplate >& GetPushConstantBuffer() { return m_PushConstants; }
		const std::vector< ShaderPushConstantTemplate >& GetPushConstantBuffer() const { return m_PushConstants; }

		std::vector< VkPushConstantRange >& GetPushConstantRanges() { return m_VulkanRanges; }
		const std::vector< VkPushConstantRange  >& GetPushConstantRanges() const { return m_VulkanRanges; }

		size_t GetDescriptorSetCount() const { return m_DescriptorSets.size(); }

		Ref<DescriptorSet> CreateDescriptorSet( uint32_t set, bool UseRendererPool = false );
		VkDescriptorSet AllocateDescriptorSet( uint32_t set, bool UseRendererPool = false );

		ShaderDescriptorSetTemplate& GetShaderDescriptorSetTemplates( uint32_t set ) { return m_DescriptorSets[ set ]; }

		std::vector< VkDescriptorSetLayout > GetSetLayouts();
		inline VkDescriptorSetLayout GetSetLayout( uint32_t set = 0 ) { return m_DescriptorSets[ set ].SetLayout; }

		void SerialiseShaderData( std::ofstream& rStream ) const;
		void DeserialiseShaderData( std::ifstream& rStream );

		[[nodiscard]] bool TryRecompile();

		const UUID GetShaderHash() const;

	private:
		void ReadFile();

		void DetermineShaderTypes();
		
		void Reflect( ShaderType shaderType, const std::vector<uint32_t>& rShaderData );
		
		void CreateDescriptors();

		[[nodiscard]] bool CompileGlslToSpvAssembly();

	private:
		SpvSourceMap m_SpvCode;

		std::string m_Name = "";

#if !defined(SAT_DIST)
		std::string m_FileContents = "";
		size_t m_FileSize = 0;

		std::filesystem::path m_Filepath = "";

		ShaderSourceMap m_ShaderSources;
#endif
		// Set -> ShaderDescriptorSet
		std::unordered_map< uint32_t, ShaderDescriptorSetTemplate > m_DescriptorSets;
		std::vector<ShaderPushConstantTemplate> m_PushConstants;
		std::vector<VkPushConstantRange> m_VulkanRanges;

		Ref<DescriptorPool> m_SetPool;
		UUID m_ShaderHash;

	private:
		friend class ShaderBundle;
	};

	// The shader library will hold shaders
	class ShaderLibrary : public RefTarget
	{
	public:
		SAT_SINGLETON_LAZY( ShaderLibrary )

	public:
		ShaderLibrary();
		~ShaderLibrary();
		
		void Add( const Ref<Shader>& shader, bool override = false );
		void Load( const std::string& path );
		void Load( const std::string& name, const std::string& path );
		void Remove( const Ref<Shader>& shader );

		// If the shader does not exist, it will load it.
		const Ref<Shader>& FindOrLoad( const std::string& name, const std::string& path );
		
		Ref<Shader> Find( const std::string& name );

		std::unordered_map<std::string, Ref<Shader>>& GetShaders() { return m_Shaders; }
		const std::unordered_map<std::string, Ref<Shader>>& GetShaders() const { return m_Shaders; }

		void Shutdown();
	private:
		std::unordered_map<std::string, Ref<Shader>> m_Shaders;
	};
}