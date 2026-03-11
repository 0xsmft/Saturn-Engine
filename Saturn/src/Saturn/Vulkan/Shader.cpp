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
#include "Shader.h"

#include "DescriptorSet.h"

#include "VulkanContext.h"
#include "Renderer.h"

#include "Saturn/Serialisation/Raw/RawSerialisation.h"

#if !defined( SAT_DIST )
#include <shaderc/shaderc.hpp>
#include <shaderc/shaderc.h>

#include <spirv/spirv_glsl.hpp>
#endif

#if !defined( SAT_DIST )
#define SHADER_INFO(...) SAT_CORE_INFO(__VA_ARGS__)
#else
#define SHADER_INFO(...)
#endif

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

	static VkShaderStageFlags ShaderTypeToVulkan( ShaderType type ) 
	{
		switch( type )
		{
			case Saturn::ShaderType::None:
				return VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;
			case Saturn::ShaderType::Vertex:
				return VK_SHADER_STAGE_VERTEX_BIT;
			case Saturn::ShaderType::Fragment:
				return VK_SHADER_STAGE_FRAGMENT_BIT;
			case Saturn::ShaderType::Geometry:
				return VK_SHADER_STAGE_GEOMETRY_BIT;
			case Saturn::ShaderType::Compute:
				return VK_SHADER_STAGE_COMPUTE_BIT;
			case Saturn::ShaderType::All:
				return VK_SHADER_STAGE_ALL;
		}

		return VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;
	}

#if !defined(SAT_DIST)
	static ShaderDataType SpvToSaturn( spirv_cross::SPIRType type )
	{
		switch( type.basetype )
		{
			case spirv_cross::SPIRType::Boolean: return ShaderDataType::Bool;
			case spirv_cross::SPIRType::Int: return ShaderDataType::Int;

			case spirv_cross::SPIRType::Float: 
			{
				if( type.vecsize == 1 ) return ShaderDataType::Int;
				if( type.vecsize == 2 ) return ShaderDataType::Int2;
				if( type.vecsize == 3 ) return ShaderDataType::Int3;
				if( type.vecsize == 4 ) return ShaderDataType::Int4;

				if( type.columns == 3 ) return ShaderDataType::Mat3;
				if( type.columns == 4 ) return ShaderDataType::Mat4;
			} break;

			case spirv_cross::SPIRType::SampledImage: return ShaderDataType::Sampler2D;
		}

		return ShaderDataType::None;
	}
#endif

	static ShaderType ShaderTypeFromString( const std::string& Str )
	{
		if( Str == "vertex" )
		{
			return ShaderType::Vertex;
		}
		else if( Str == "fragment" )
		{
			return ShaderType::Fragment;
		}
		else if( Str == "compute" )
		{
			return ShaderType::Compute;
		}
		else if( Str == "geometry" )
		{
			return ShaderType::Geometry;
		}
		else
		{
			return ShaderType::None;
		}
	}

	static std::string ShaderTypeToString( ShaderType Type )
	{
		switch( Type )
		{
			case Saturn::ShaderType::Vertex:
				return "Vertex";
			case Saturn::ShaderType::Fragment:
				return "Fragment";
			case Saturn::ShaderType::Geometry:
				return "Geometry";
			case Saturn::ShaderType::Compute:
				return "Compute";
			default:
				break;
		}

		return "";
	}

	//////////////////////////////////////////////////////////////////////////
	// SHADER LIBRARY

	ShaderLibrary::ShaderLibrary()
	{
	}

	ShaderLibrary::~ShaderLibrary()
	{
		Shutdown();
	}

	void ShaderLibrary::Add( const Ref<Shader>& shader, bool override /* =false*/ )
	{
		auto& rName = shader->GetName();
		
		if( m_Shaders.find( rName ) == m_Shaders.end() ) 
		{
			m_Shaders[ rName ] = shader;
		}
		else if( override )
		{
			SAT_CORE_WARN("Shader already exists and \"override\" was set overriding shader...");
			
			m_Shaders[ rName ] = shader;
		}
	}

	void ShaderLibrary::Load( const std::string& path )
	{
		if( Find( path ) != nullptr )
			return;

		Ref<Shader> shader = Ref<Shader>::Create( path );
	
		Add( shader );
	}

	void ShaderLibrary::Load( const std::string& name, const std::string& path )
	{
#if !defined(SAT_DIST)
		SAT_CORE_ASSERT( m_Shaders.find( name ) == m_Shaders.end() );

		m_Shaders[ name ] = Ref<Shader>::Create( path );
#else
		SAT_CORE_VERIFY( m_Shaders.find( name ) != m_Shaders.end(), "Shader is unable to be found in the ShaderBundle! It must exist! Dist cannot load shaders from disk. This may indicate that the ShaderBuundle is outdated! Please rebuild the it." );
#endif
	}

	void ShaderLibrary::Remove( const Ref<Shader>& shader )
	{
		m_Shaders[ shader->GetName() ] = nullptr;
		m_Shaders.erase( shader->GetName() );
	}

	const Ref<Shader>& ShaderLibrary::FindOrLoad( const std::string& name, const std::string& path )
	{
		// Does the shader exists?
		if( m_Shaders.find( name ) == m_Shaders.end() ) 
		{
			// No, we'll add it.
			Load( name, path );
		}

		return m_Shaders.at( name );
	}

	void ShaderLibrary::Shutdown()
	{
		for( auto& [name, shader] : m_Shaders )
		{
			if( shader )
				shader = nullptr;
		}

		m_Shaders.clear();
	}

	Ref<Shader> ShaderLibrary::Find( const std::string& name )
	{
		if( m_Shaders.find( name ) == m_Shaders.end() ) 
		{
			SAT_CORE_ERROR( "Failed to find shader \"{0}\"", name );
			SAT_CORE_ASSERT(false);

			return nullptr;
		}

		return m_Shaders.at( name );
	}
	
	//////////////////////////////////////////////////////////////////////////
	// SHADER

	Shader::Shader( const std::filesystem::path& rFilepath )
#if !defined(SAT_DIST)
		: m_Filepath( rFilepath )
	{
		if( !std::filesystem::exists( m_Filepath ) )
			return;

		m_Name = m_Filepath.stem().string();

		ReadFile();
		DetermineShaderTypes();

		if( !CompileGlslToSpvAssembly() ) 
		{
			SAT_CORE_ERROR( "Shader failed to compile!" );
			SAT_CORE_ASSERT( false );
			
			return;
		}

		for( const auto& [k, data] : m_SpvCode )
		{
			Reflect( k.Type, data );
		}

		CreateDescriptors();
	
		Renderer::Get()->AddShaderReference( m_ShaderHash );
	}
#else
	{
	}
#endif

	Shader::~Shader()
	{
		m_SpvCode.clear();
#if !defined(SAT_DIST)
		m_ShaderSources.clear();
#endif
		for( auto& [ set, descriptorSet ] : m_DescriptorSets )
		{
			vkDestroyDescriptorSetLayout( VulkanContext::Get()->GetDevice(), descriptorSet.SetLayout, nullptr );
		}

		m_SetPool = nullptr;

#if !defined(SAT_DIST)
		Renderer::Get()->RemoveShaderReference( m_ShaderHash );
#endif
	}

	void Shader::WriteDescriptor( const std::string& rName, VkDescriptorImageInfo& rImageInfo, VkDescriptorSet desSet )
	{
		for( auto& [set, descriptorSet] : m_DescriptorSets )
		{
			for( auto& texture : descriptorSet.SampledImages )
			{
				if( texture.Name == rName )
				{
					descriptorSet.WriteDescriptorSets[ texture.Binding ].pImageInfo = &rImageInfo;
					descriptorSet.WriteDescriptorSets[ texture.Binding ].dstSet = desSet;

					vkUpdateDescriptorSets( VulkanContext::Get()->GetDevice(), 1, &descriptorSet.WriteDescriptorSets[ texture.Binding ], 0, nullptr );
				}
			}

			for( auto& texture : descriptorSet.StorageImages )
			{
				if( texture.Name == rName )
				{
					descriptorSet.WriteDescriptorSets[ texture.Binding ].pImageInfo = &rImageInfo;
					descriptorSet.WriteDescriptorSets[ texture.Binding ].dstSet = desSet;

					vkUpdateDescriptorSets( VulkanContext::Get()->GetDevice(), 1, &descriptorSet.WriteDescriptorSets[ texture.Binding ], 0, nullptr );
				}
			}
		}
	}

	Ref<DescriptorSet> Shader::CreateDescriptorSet( uint32_t set, bool UseRendererPool /*= false */ )
	{
		DescriptorSetSpecification Specification;
		Specification.Layout = m_DescriptorSets[ set ].SetLayout;
		Specification.Pool = UseRendererPool ? Renderer::Get()->GetDescriptorPool() : m_SetPool;
		Specification.SetIndex = set;

		return Ref<DescriptorSet>::Create( Specification );
	}

	VkDescriptorSet Shader::AllocateDescriptorSet( uint32_t set, bool UseRendererPool /*= false */ )
	{
		VkDescriptorSet Set = VK_NULL_HANDLE;

		VkDescriptorSetAllocateInfo AllocateInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
		AllocateInfo.descriptorPool = UseRendererPool ? Renderer::Get()->GetDescriptorPool()->GetVulkanPool() : m_SetPool->GetVulkanPool();
		AllocateInfo.descriptorSetCount = 1;
		AllocateInfo.pSetLayouts = &m_DescriptorSets[ set ].SetLayout;

		VK_CHECK( vkAllocateDescriptorSets( VulkanContext::Get()->GetDevice(), &AllocateInfo, &Set ) );

		return Set;
	}

	std::vector< VkDescriptorSetLayout > Shader::GetSetLayouts()
	{
		std::vector< VkDescriptorSetLayout > layouts;

		for( auto& [set, layout] : m_DescriptorSets )
			layouts.push_back( layout.SetLayout );

		return layouts;
	}

	void Shader::ReadFile()
	{
#if !defined(SAT_DIST)
		// Read the raw GLSL text file.
		std::ifstream stream( m_Filepath, std::ios::ate | std::ios::binary );
		if( !stream.is_open() )
		{
			SAT_CORE_ERROR( "Failed to open shader file \"{0}\"", m_Filepath.string() );
			SAT_CORE_ASSERT( false );
		}

		m_FileSize = ( size_t ) stream.tellg();
		m_FileContents.resize( m_FileSize );

		stream.seekg( 0 );
		stream.read( m_FileContents.data(), m_FileSize );
		stream.close();
#endif
	}

	void Shader::DetermineShaderTypes()
	{
#if !defined(SAT_DIST)
		int VertexShaders = -1;
		int FragmentShaders = -1;
		int ComputeShaders = -1;

		const char* TypeToken = "#type";
		const size_t TypeTokenLength = strlen( TypeToken );
		size_t TypeTokenPosition = m_FileContents.find( TypeToken, 0 );
		
		while ( TypeTokenPosition != std::string::npos )
		{
			std::string FileCopy;
			
			std::copy( m_FileContents.begin(), m_FileContents.end(), std::back_inserter( FileCopy ) );
			
			const size_t TypeTokenEnd = FileCopy.find( "\r\n", TypeTokenPosition );

			const size_t Begin = TypeTokenPosition + TypeTokenLength + 1;

			const std::string Type = FileCopy.substr( Begin, TypeTokenEnd - Begin );
			
			const size_t NextLinePos = FileCopy.find_first_not_of( "\r\n", TypeTokenEnd );
			TypeTokenPosition = FileCopy.find( TypeToken, NextLinePos );
			
			const auto RawShaderCode = FileCopy.substr( NextLinePos, TypeTokenPosition - ( NextLinePos == std::string::npos ? FileCopy.size() - 1 : NextLinePos ) );

			const auto Shader_Type = ShaderTypeFromString( Type );

			if( Shader_Type == ShaderType::Fragment )
				++FragmentShaders;
			else if( Shader_Type == ShaderType::Vertex )
				++VertexShaders;
			else if( Shader_Type == ShaderType::Compute )
				++ComputeShaders;
			
			const int Index = Shader_Type == ShaderType::Vertex ? VertexShaders : ( Shader_Type == ShaderType::Fragment ? FragmentShaders : ComputeShaders );

			ShaderSource src = ShaderSource( RawShaderCode, Shader_Type, Index );
			m_ShaderSources[ ShaderSourceKey( Shader_Type, Index ) ] = src;
		}
#endif
	}

	void Shader::Reflect( ShaderType shaderType, const std::vector<uint32_t>& rShaderData )
	{
#if !defined(SAT_DIST)
		spirv_cross::Compiler Compiler( rShaderData );
		auto Resources = Compiler.get_shader_resources();

		// Sort the descriptors by set and binding.
		auto Fn = [&]( const auto& a, const auto& b ) -> bool
		{
			const uint32_t aSet = Compiler.get_decoration( a.id, spv::DecorationDescriptorSet );
			const uint32_t bSet = Compiler.get_decoration( b.id, spv::DecorationDescriptorSet );

			if( aSet == bSet )
			{
				const uint32_t aBinding = Compiler.get_decoration( a.id, spv::DecorationBinding );
				const uint32_t bBinding = Compiler.get_decoration( b.id, spv::DecorationBinding );

				return aBinding < bBinding;
			}

			return aSet < bSet;
		};

		for( const auto& sb : Resources.storage_buffers )
		{
			const auto& Name = sb.name;
			auto& BufferType = Compiler.get_type( sb.base_type_id );
			int MemberCount = ( int ) BufferType.member_types.size();
			uint32_t Binding = Compiler.get_decoration( sb.id, spv::DecorationBinding );
			uint32_t Set = Compiler.get_decoration( sb.id, spv::DecorationDescriptorSet );

			uint32_t Size = ( uint32_t ) Compiler.get_declared_struct_size( BufferType );

			SHADER_INFO( "Storage Buffer: {0}", Name );
			SHADER_INFO( " Size: {0}", Size );
			SHADER_INFO( " Binding: {0}", Binding );
			SHADER_INFO( " Set: {0}", Set );

			if( m_DescriptorSets[ Set ].Set == -1 )
				m_DescriptorSets[ Set ] = ShaderDescriptorSetTemplate( Set );

			ShaderStorageBuffer Storage;
			Storage.Binding = Binding;
			Storage.Size = 1;
			Storage.Location = shaderType;
			Storage.Name = Name;

			auto& storageBuffers = m_DescriptorSets[ Set ].StorageBuffers;

			// Check if the same element already exists if so the stage "All"
			// is used to represent all stages.
			auto It = std::find_if( storageBuffers.begin(), storageBuffers.end(), [ & ]( const auto& a )
			{
				auto&& [k, v] = a;

				return v == Storage;
			} );

			if( It != std::end( storageBuffers ) )
			{
				auto&& [Binding, sb] = ( *It );

				sb.Location = ShaderType::All;

				continue;
			}

			m_DescriptorSets[ Set ].StorageBuffers[ Binding ] = Storage;

			for( int i = 0; i < MemberCount; ++i )
			{
				auto type = Compiler.get_type( BufferType.member_types[ i ] );
				const auto& memberName = Compiler.get_member_name( BufferType.self, i );
				size_t size = Compiler.get_declared_struct_member_size( BufferType, i );
				auto offset = Compiler.type_struct_member_offset( BufferType, i );

				std::string MemberName = Name + "." + memberName;

				SHADER_INFO( "  {0}", memberName );
				SHADER_INFO( "   Size: {0}", size );
				SHADER_INFO( "   Offset: {0}", offset );

				// Use Binding as location it does not matter.
//				m_Uniforms.push_back( { MemberName, ( int ) Binding, SpvToSaturn( type ), size, offset } );
			}
		}

		for ( const auto& ub : Resources.uniform_buffers )
		{
			const auto& Name = ub.name;
			auto& BufferType = Compiler.get_type( ub.base_type_id );
			int MemberCount = ( int ) BufferType.member_types.size();
			uint32_t Binding = Compiler.get_decoration( ub.id, spv::DecorationBinding );
			uint32_t Set = Compiler.get_decoration( ub.id, spv::DecorationDescriptorSet );

			uint32_t Size = ( uint32_t ) Compiler.get_declared_struct_size( BufferType );

			SHADER_INFO( "Uniform Buffer: {0}", Name );
			SHADER_INFO( " Size: {0}", Size );
			SHADER_INFO( " Binding: {0}", Binding );
			SHADER_INFO( " Set: {0}", Set);

			if( m_DescriptorSets[ Set ].Set == -1 )
				m_DescriptorSets[ Set ] = ShaderDescriptorSetTemplate( Set );

			ShaderUniformBuffer Uniform;
			Uniform.Binding = Binding;
			Uniform.Size = Size;
			Uniform.Location = shaderType;
			Uniform.Name = Name;

			auto& UniformBuffers = m_DescriptorSets[ Set ].UniformBuffers;

			// Check if the same element already exists if so the stage "All"
			// is used to represent all stages.
			auto It = std::find_if( UniformBuffers.begin(), UniformBuffers.end(), [&]( const auto& a )
			{
				auto&& [k, v] = a;

				return v == Uniform;
			} );
			
			if( It != std::end( UniformBuffers ) )
			{
				auto&& [Binding, ub] = ( *It );

				ub.Location = ShaderType::All;

				continue;
			}

			m_DescriptorSets[ Set ].UniformBuffers[ Binding ] = Uniform;

			for( int i = 0; i < MemberCount; ++i )
			{
				auto& type = Compiler.get_type( BufferType.member_types[i] );
				const auto& memberName = Compiler.get_member_name( BufferType.self, i );
				size_t size = Compiler.get_declared_struct_member_size( BufferType, i );
				auto offset = Compiler.type_struct_member_offset( BufferType, i );

				std::string MemberName = Name + "." + memberName;

				SHADER_INFO( "  {0}", memberName );
				SHADER_INFO( "   Size: {0}", size );
				SHADER_INFO( "   Offset: {0}", offset );

				// Use Binding as location it does not matter.
//				m_Uniforms.push_back( { MemberName, (int)Binding, SpvToSaturn( type ), size, offset } );
			}
		}

		for ( const auto& pc : Resources.push_constant_buffers )
		{
			const auto& Name = pc.name;
			auto& BufferType = Compiler.get_type( pc.base_type_id );
			int MemberCount = ( int )BufferType.member_types.size();
			uint32_t set = Compiler.get_decoration( pc.id, spv::DecorationDescriptorSet );

			uint32_t Size = ( uint32_t ) Compiler.get_declared_struct_size( BufferType );
			uint32_t OffsetFromLastPC = 0;

			// Make sure the buffer offset is size + offset for because that's what vulkan needs when we render.
			if( m_VulkanRanges.size() )
				OffsetFromLastPC = m_VulkanRanges.back().offset + m_VulkanRanges.back().size;

			ShaderPushConstantTemplate pc;
			pc.Name = Name;
			pc.Size = Size;
			pc.Stage = shaderType;
			m_VulkanRanges.push_back( { .stageFlags = ShaderTypeToVulkan( shaderType ), .offset = OffsetFromLastPC , .size = ( uint32_t ) Size } );

			SHADER_INFO( "Push constant buffer: {0}", Name );
			SHADER_INFO( " Size: {0}", Size );
			SHADER_INFO( " Offset: {0}", OffsetFromLastPC );
			SHADER_INFO( " Set: {0}", ( uint32_t ) set );
			SHADER_INFO( " Stage: {0}", ( uint32_t )shaderType );

			for( int i = 0; i < MemberCount; i++ )
			{
				auto type = Compiler.get_type( BufferType.member_types[ i ] );
				const auto& memberName = Compiler.get_member_name( BufferType.self, i );
				size_t size = Compiler.get_declared_struct_member_size( BufferType, i );
				auto offset = Compiler.type_struct_member_offset( BufferType, i );
				
				std::string MemberName;

				if( Name.empty() )
					MemberName = memberName;
				else
					MemberName = Name + "." + memberName;

				/*
				bool PushConstantData = false;

				if( shaderType == ShaderType::Fragment )
					PushConstantData = true;
				*/

				SHADER_INFO( "  {0}", memberName );
				SHADER_INFO( "  Size: {0}", size );
				SHADER_INFO( "  Offset: {0}", offset );

				pc.MemberOffsets[ MemberName ] = offset - OffsetFromLastPC;
			}

			m_PushConstants.push_back( pc );
		}

		for( const auto& Resource : Resources.sampled_images )
		{
			const auto& Name = Resource.name;
			auto& BaseType = Compiler.get_type( Resource.base_type_id );
			auto& RealType = Compiler.get_type( Resource.type_id );

			uint32_t binding = Compiler.get_decoration( Resource.id, spv::DecorationBinding );
			uint32_t set = Compiler.get_decoration( Resource.id, spv::DecorationDescriptorSet );
			uint32_t arraySizes = RealType.array[ 0 ];

			SHADER_INFO( "Sampled image: {0}", Name );
			SHADER_INFO( " Binding: {0}", binding );
			SHADER_INFO( " Set: {0}", set );

			if( arraySizes == 0 )
				arraySizes = 1;

			if( m_DescriptorSets[ set ].Set == UINT32_MAX )
				m_DescriptorSets[ set ] = ShaderDescriptorSetTemplate( set );

			m_DescriptorSets[ set ].SampledImages.push_back( { Name, shaderType, set, binding, arraySizes } );
		}

		for ( const auto& Resource : Resources.storage_images )
		{
			const auto& Name = Resource.name;
			auto& BaseType = Compiler.get_type( Resource.base_type_id );
			auto& RealType = Compiler.get_type( Resource.type_id );

			uint32_t binding = Compiler.get_decoration( Resource.id, spv::DecorationBinding );
			uint32_t set = Compiler.get_decoration( Resource.id, spv::DecorationDescriptorSet );
			uint32_t arraySizes = RealType.array[ 0 ];

			SHADER_INFO( "Storage image: {0}", Name );
			SHADER_INFO( " Binding: {0}", binding );
			SHADER_INFO( " Set: {0}", set );

			if( arraySizes == 0 )
				arraySizes = 1;

			if( m_DescriptorSets[ set ].Set == UINT32_MAX )
				m_DescriptorSets[ set ] = ShaderDescriptorSetTemplate( set );

			m_DescriptorSets[ set ].StorageImages.push_back( { Name, shaderType, set, binding, arraySizes } );
		}
#endif
	}

	void Shader::CreateDescriptors()
	{
		// Create the descriptor set layout.
		std::vector< VkDescriptorPoolSize > PoolSizes;

		// Iterate over descriptor sets
		for( auto& [ set, descriptorSet ] : m_DescriptorSets )
		{
			std::vector< VkDescriptorSetLayoutBinding > Bindings;

			// Iterate over uniform buffers
			for( auto& [ Binding, ub ] : descriptorSet.UniformBuffers )
			{
				VkDescriptorSetLayoutBinding Binding = {};
				Binding.binding = ub.Binding;
				Binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				Binding.descriptorCount = 1;
				Binding.stageFlags = ub.Location == ShaderType::Vertex ? VK_SHADER_STAGE_VERTEX_BIT : ub.Location == ShaderType::All ? VK_SHADER_STAGE_ALL : ub.Location == ShaderType::Compute ? VK_SHADER_STAGE_COMPUTE_BIT : VK_SHADER_STAGE_FRAGMENT_BIT;
				Binding.pImmutableSamplers = nullptr;

				PoolSizes.emplace_back( VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 250 );

				Bindings.push_back( Binding );

				descriptorSet.WriteDescriptorSets[ ub.Binding ] =
				{
					.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
					.pNext = nullptr,
					.dstSet = nullptr,
					.dstBinding = ub.Binding,
					.dstArrayElement = 0,
					.descriptorCount = 1,
					.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
					.pImageInfo = nullptr,
					.pBufferInfo = nullptr,
					.pTexelBufferView = nullptr
				};
			}

			// Iterate over storage buffers
			for( auto& [Binding, sb] : descriptorSet.StorageBuffers )
			{
				VkDescriptorSetLayoutBinding Binding = {};
				Binding.binding = sb.Binding;
				Binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
				Binding.descriptorCount = 1;
				Binding.stageFlags = sb.Location == ShaderType::Vertex ? VK_SHADER_STAGE_VERTEX_BIT : sb.Location == ShaderType::All ? VK_SHADER_STAGE_ALL : sb.Location == ShaderType::Compute ? VK_SHADER_STAGE_COMPUTE_BIT : VK_SHADER_STAGE_FRAGMENT_BIT;
				Binding.pImmutableSamplers = nullptr;

				PoolSizes.emplace_back( VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 250 );

				Bindings.push_back( Binding );

				descriptorSet.WriteDescriptorSets[ sb.Binding ] =
				{
					.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
					.pNext = nullptr,
					.dstSet = nullptr,
					.dstBinding = sb.Binding,
					.dstArrayElement = 0,
					.descriptorCount = 1,
					.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
					.pImageInfo = nullptr,
					.pBufferInfo = nullptr,
					.pTexelBufferView = nullptr
				};
			}

			// Iterate over textures
			for( auto& texture : descriptorSet.SampledImages )
			{
				VkDescriptorSetLayoutBinding Binding = {};
				Binding.binding = texture.Binding;
				Binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				Binding.descriptorCount = texture.ArraySize;
				Binding.stageFlags = texture.Stage == ShaderType::Vertex ? VK_SHADER_STAGE_VERTEX_BIT : ( texture.Stage == ShaderType::All ? VK_SHADER_STAGE_ALL : ( texture.Stage == ShaderType::Compute ? VK_SHADER_STAGE_COMPUTE_BIT : VK_SHADER_STAGE_FRAGMENT_BIT ) );
				Binding.pImmutableSamplers = nullptr;

				PoolSizes.emplace_back( VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 250 );

				Bindings.push_back( Binding );

				descriptorSet.WriteDescriptorSets[ texture.Binding ] =
				{
					.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
					.pNext = nullptr,
					.dstSet = nullptr,
					.dstBinding = texture.Binding,
					.dstArrayElement = 0,
					.descriptorCount = 1,
					.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
					.pImageInfo = nullptr,
					.pBufferInfo = nullptr,
					.pTexelBufferView = nullptr
				};
			}

			// Storage images
			for( auto& texture : descriptorSet.StorageImages )
			{
				VkDescriptorSetLayoutBinding Binding = {};
				Binding.binding = texture.Binding;
				Binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
				Binding.descriptorCount = texture.ArraySize;
				Binding.stageFlags = texture.Stage == ShaderType::Vertex ? VK_SHADER_STAGE_VERTEX_BIT : ( texture.Stage == ShaderType::All ? VK_SHADER_STAGE_ALL : ( texture.Stage == ShaderType::Compute ? VK_SHADER_STAGE_COMPUTE_BIT : VK_SHADER_STAGE_FRAGMENT_BIT ) );
				Binding.pImmutableSamplers = nullptr;

				PoolSizes.emplace_back( VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 250 );

				Bindings.push_back( Binding );

				descriptorSet.WriteDescriptorSets[ texture.Binding ] =
				{
					.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
					.pNext = nullptr,
					.dstSet = nullptr,
					.dstBinding = texture.Binding,
					.dstArrayElement = 0,
					.descriptorCount = 1,
					.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
					.pImageInfo = nullptr,
					.pBufferInfo = nullptr,
					.pTexelBufferView = nullptr
				};
			}

			VkDescriptorSetLayoutCreateInfo LayoutInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
			LayoutInfo.bindingCount = ( uint32_t ) Bindings.size();
			LayoutInfo.pBindings = Bindings.data();

			VK_CHECK( vkCreateDescriptorSetLayout( VulkanContext::Get()->GetDevice(), &LayoutInfo, nullptr, &descriptorSet.SetLayout ) );
		}

		m_SetPool = Ref< DescriptorPool >::Create( PoolSizes, 10000 );
	}

	bool Shader::CompileGlslToSpvAssembly()
	{
#if !defined(SAT_DIST)
		shaderc::Compiler       Compiler;
		shaderc::CompileOptions CompilerOptions;

		// We only use shaderc_optimization_level_zero, if not it will remove the uniform names, it took me 6 hours to figure out.
		CompilerOptions.SetOptimizationLevel( shaderc_optimization_level_zero );

		CompilerOptions.SetWarningsAsErrors();

		CompilerOptions.SetTargetEnvironment( shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_2 );
		CompilerOptions.SetTargetSpirv( shaderc_spirv_version_1_5 );

		// TODO: Shader cache and shader hot reloading.
		Timer CompileTimer;
	
		for ( auto&& [key, src] : m_ShaderSources )
		{
			const std::string& rShaderSrcCode = src.Source;

			const auto Res = Compiler.CompileGlslToSpv(
				rShaderSrcCode,
				src.Type == ShaderType::Vertex ? shaderc_shader_kind::shaderc_glsl_default_vertex_shader : src.Type == ShaderType::Compute ? shaderc_shader_kind::shaderc_glsl_default_compute_shader : shaderc_shader_kind::shaderc_glsl_default_fragment_shader,
				m_Filepath.string().c_str(),
				CompilerOptions );

			if( Res.GetCompilationStatus() != shaderc_compilation_status_success ) 
			{
				SAT_CORE_ERROR( "Shader Error status {0}", Res.GetCompilationStatus() );
				SAT_CORE_ERROR( "Shader Error at shader stage: {0}", ShaderTypeToString( key.Type ) );
				SAT_CORE_ERROR( "Shader Error messages {0}", Res.GetErrorMessage() );
				return false;
			}

			SHADER_INFO( "Shader Warnings {0}", Res.GetNumWarnings() );

			std::vector< uint32_t > SpvBinary( Res.begin(), Res.end() );

			m_SpvCode[ key ] = SpvBinary;
		}

		SHADER_INFO( "Shader Compilation took {0} ms", CompileTimer.ElapsedMilliseconds() );
		
		return true;
#else
		return false;
#endif
	}

	bool Shader::TryRecompile()
	{
#if !defined(SAT_DIST)
		// Create copies of our data so if anything goes wrong we can set it back to what it was before.
		std::string OldfileContents = m_FileContents;
		size_t OldFileSize = m_FileSize;

		auto OldSpvMap          = m_SpvCode;
		auto OldDescriptorSets  = m_DescriptorSets;
		auto OldPushConstsRange = m_VulkanRanges;
		auto OldPushContsts = m_PushConstants;

		// Read the updated file.
		ReadFile();
		DetermineShaderTypes();

		// Clear descriptors and push consts.
		m_SpvCode.clear();
		m_DescriptorSets.clear();
		m_VulkanRanges.clear();
		m_PushConstants.clear();

		if( !CompileGlslToSpvAssembly() )
		{
			m_SpvCode              = OldSpvMap;
			m_DescriptorSets       = OldDescriptorSets;
			m_VulkanRanges         = OldPushConstsRange;
			m_PushConstants        = OldPushContsts;

			SAT_CORE_ERROR( "Shader hot reloading failed. Shader did not compile successfully!" );

			return false;
		}

		for( const auto& [k, data] : m_SpvCode )
		{
			Reflect( k.Type, data );
		}

		CreateDescriptors();

		Renderer::Get()->OnShaderReloaded( m_Name );

		return true;
#else
		return false;
#endif
	}

	const UUID Shader::GetShaderHash() const
	{
		return m_ShaderHash;
	}

	//////////////////////////////////////////////////////////////////////////
	// Distribution shader serialisation

	void Shader::SerialiseShaderData( std::ofstream& rStream ) const
	{
		RawSerialisation::WriteUnorderedMap( m_SpvCode, rStream );
		RawSerialisation::WriteMap( m_DescriptorSets, rStream );

		RawSerialisation::WriteVector( m_PushConstants, rStream );
		RawSerialisation::WriteVector( m_VulkanRanges, rStream );
	}

	void Shader::DeserialiseShaderData( std::ifstream& rStream )
	{
		RawSerialisation::ReadUnorderedMap( m_SpvCode, rStream );
		RawSerialisation::ReadMap( m_DescriptorSets, rStream );

		RawSerialisation::ReadVector( m_PushConstants, rStream );
		RawSerialisation::ReadVector( m_VulkanRanges, rStream );

		// Clean up some of the data that was read.
		for( auto&& [k, v] : m_DescriptorSets )
			v.SetLayout = nullptr;

		CreateDescriptors();
	}

}
