/********************************************************************************************
*                                                                                           *
*                                                                                           *
*                                                                                           *
* MIT License                                                                               *
*                                                                                           *
* Copyright (c) 2020 - 2024 BEAST                                                           *
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

#include "ShaderUniform.h"
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
	
	// A shader uniform buffer represents a uniform buffer block in a shader.
	struct ShaderUniformBuffer
	{
		std::string Name;
		uint32_t Binding;
		size_t Size = 0;
		ShaderType Location = ShaderType::None;
		
		VkBuffer Buffer = VK_NULL_HANDLE;

		bool operator==( const ShaderUniformBuffer& rOther ) 
		{
			return Binding == rOther.Binding && Name == rOther.Name && Size == rOther.Size;
		}

		auto operator<=>( const ShaderUniformBuffer& rOther ) const = default;

		std::vector< ShaderUniform > Members;

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

	struct ShaderStorageBuffer
	{
		std::string Name;
		uint32_t Binding;
		size_t Size = 0;
		ShaderType Location = ShaderType::None;
		bool Updated = false;

		bool operator==( const ShaderStorageBuffer& rOther )
		{
			return Binding == rOther.Binding && Name == rOther.Name && Size == rOther.Size;
		}

		auto operator<=>( const ShaderStorageBuffer& rOther ) const = default;

		std::vector< ShaderUniform > Members;

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

	struct ShaderDescriptorSet
	{
		uint32_t Set;

		VkDescriptorSetLayout SetLayout = nullptr;
		
		std::unordered_map< uint32_t, VkWriteDescriptorSet > WriteDescriptorSets;
		std::vector< ShaderSampledImage > SampledImages;
		std::vector< ShaderSampledImage > StorageImages;
		std::unordered_map< uint32_t, ShaderUniformBuffer > UniformBuffers;
		std::unordered_map< uint32_t, ShaderStorageBuffer > StorageBuffers;

		static void Serialise( const ShaderDescriptorSet& rObject, std::ofstream& rStream )
		{
			RawSerialisation::WriteObject( rObject.Set, rStream );

			RawSerialisation::WriteVector( rObject.SampledImages, rStream );
			RawSerialisation::WriteVector( rObject.StorageImages, rStream );

			RawSerialisation::WriteUnorderedMap( rObject.UniformBuffers, rStream );
			RawSerialisation::WriteUnorderedMap( rObject.StorageBuffers, rStream );
		}

		static void Deserialise( ShaderDescriptorSet& rObject, std::ifstream& rStream )
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

	static ShaderType VulkanStageToSaturn( VkShaderStageFlags Flags )
	{
		switch( Flags )
		{
			case VK_SHADER_STAGE_VERTEX_BIT:
				return ShaderType::Vertex;
			case VK_SHADER_STAGE_FRAGMENT_BIT:
				return ShaderType::Fragment;
			case VK_SHADER_STAGE_GEOMETRY_BIT:
				return ShaderType::Geometry;
			case VK_SHADER_STAGE_COMPUTE_BIT:
				return ShaderType::Compute;
			default:
				return ShaderType::All;
		}
	}
	
	extern ShaderType ShaderTypeFromString( const std::string& Str );
	extern std::string ShaderTypeToString( ShaderType Type );

	class Shader : public RefTarget
	{
		using ShaderSourceMap = std::unordered_map< ShaderSourceKey, ShaderSource >;
		using SpvSourceMap = std::unordered_map< ShaderSourceKey, std::vector< uint32_t > >;
		using ShaderUBMap = std::unordered_map< ShaderType, std::unordered_map< uint32_t, ShaderUniformBuffer > >;
		//                                      ShaderType,                     Binding,   UniformBuffer
		
		using ShaderWriteMap = std::unordered_map< ShaderType, std::unordered_map< std::string, VkWriteDescriptorSet > >;


	public:
		// Internal default constructor, only used when reading from a shader bundle.
		// Do not use!
		Shader() {}
	
		Shader( const std::filesystem::path& rFilepath );
		~Shader();
		
		std::string& GetName() { return m_Name; }
		const std::string& GetName() const { return m_Name; }

		ShaderSourceMap& GetShaderSources() { return m_ShaderSources; }
		const ShaderSourceMap& GetShaderSources() const { return m_ShaderSources; }

		const SpvSourceMap& GetSpvCode() const { return m_SpvCode; }
		SpvSourceMap& GetSpvCode() { return m_SpvCode; }
		
		const std::vector< ShaderUniform >& GetUniforms() const { return m_Uniforms; }
		
		std::vector< ShaderSampledImage >& GetTextures() { return m_Textures; }
		
		Ref< DescriptorPool >& GetDescriptorPool() { return m_SetPool; }
		const Ref< DescriptorPool >& GetDescriptorPool() const { return m_SetPool; }
		
		ShaderWriteMap& GetWriteDescriptors() { return m_DescriptorWrites; }
		const ShaderWriteMap& GetWriteDescriptors() const { return m_DescriptorWrites; }
		
		std::vector< VkPushConstantRange >& GetPushConstantRanges() { return m_PushConstantRanges; }
		const std::vector< VkPushConstantRange >& GetPushConstantRanges() const { return m_PushConstantRanges; }

		void WriteDescriptor( const std::string& rName, VkDescriptorImageInfo& rImageInfo, VkDescriptorSet desSet );
		void WriteDescriptor( const std::string& rName, std::vector<VkDescriptorImageInfo>& rImageInfos, VkDescriptorSet desSet );

		void WriteDescriptor( const std::string& rName, VkDescriptorBufferInfo& rBufferInfo, VkDescriptorSet desSet );

		// Make sure all the buffers have data mapped to them!
		void WriteAllUBs( const Ref< DescriptorSet >& rSet );
		void WriteAllUBs( VkDescriptorSet Set );

		void WriteSB( uint32_t set, uint32_t binding, const VkDescriptorBufferInfo& rInfo, Ref<DescriptorSet>& rSet );

		void* MapUB( ShaderType Type, uint32_t Set, uint32_t Binding );
		void UnmapUB( ShaderType Type, uint32_t Set, uint32_t Binding );
		
		void UploadUB( ShaderType Type, uint32_t Set, uint32_t Binding, void* pData, size_t Size );

		uint32_t GetDescriptorSetCount() { return m_DescriptorSetCount; }

		Ref<DescriptorSet> CreateDescriptorSet( uint32_t set, bool UseRendererPool = false );
		VkDescriptorSet AllocateDescriptorSet( uint32_t set, bool UseRendererPool = false );

		ShaderDescriptorSet& GetShaderDescriptorSet( uint32_t set ) { return m_DescriptorSets[ set ]; }

		std::vector< VkDescriptorSetLayout >& GetSetLayouts() { return m_SetLayouts; }
		VkDescriptorSetLayout GetSetLayout( uint32_t set = 0 ) { return m_SetLayouts[ set ]; }

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
		ShaderSourceMap m_ShaderSources;
		SpvSourceMap m_SpvCode;

		std::string m_FileContents = "";
		size_t m_FileSize = 0;

		std::string m_Name = "";

		std::filesystem::path m_Filepath = "";
		
		std::vector< ShaderUniform > m_Uniforms;
		std::vector< ShaderUniform > m_PushConstantUniforms;
		std::vector< ShaderSampledImage > m_Textures;
		std::vector< VkPushConstantRange > m_PushConstantRanges;

		// Set -> ShaderDescriptorSet
		std::unordered_map< uint32_t, ShaderDescriptorSet > m_DescriptorSets;

		ShaderWriteMap m_DescriptorWrites;

		uint32_t m_DescriptorSetCount = -1;

		std::vector< VkDescriptorSetLayout > m_SetLayouts;

		Ref< DescriptorPool > m_SetPool;

		UUID m_ShaderHash;

	private:
		friend class ShaderBundle;
	};

	// The shader library will hold shaders
	class ShaderLibrary : public RefTarget
	{
	public:
		static inline ShaderLibrary& Get() { return *SingletonStorage::GetOrCreateSingleton<ShaderLibrary>(); }
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