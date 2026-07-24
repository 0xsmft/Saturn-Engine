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

#include "VulkanError.h"

#include "ShaderDataType.h"

#include "VulkanAllocator.h"

#include <vulkan.h>
#include <string>
#include <vector>

#include <glm/glm.hpp>

namespace Saturn {
	
	struct BaseVertex
	{
		glm::vec3 Position;
		glm::vec2 Texcoord;
	};

	struct StaticVertex
	{
		glm::vec3 Position;
		glm::vec3 Normal;
		glm::vec3 Tangent;
		glm::vec3 Binormal;
		glm::vec2 Texcoord;
	};
	
	struct VertexBufferElement
	{
		std::string Name;
		ShaderDataType Type;
		uint32_t Size;
		uint32_t Offset;

		VertexBufferElement() = default;

		VertexBufferElement( ShaderDataType type, const std::string& name )
			: Name( name ), Type( type ), Size( ShaderDataTypeSize( type ) ), Offset( 0 ) 
		{
		}

		uint32_t GetComponentCount() const
		{
			switch( Type )
			{
				case ShaderDataType::Float:       return 1;
				case ShaderDataType::Float2:      return 2;
				case ShaderDataType::Float3:      return 3;
				case ShaderDataType::Float4:      return 4;
				case ShaderDataType::Mat3:        return 3 * 3;
				case ShaderDataType::Mat4:        return 4 * 4;
				case ShaderDataType::Int:         return 1;
				case ShaderDataType::Int2:        return 2;
				case ShaderDataType::Int3:        return 3;
				case ShaderDataType::Int4:        return 4;
				case ShaderDataType::Bool:        return 1;
				case ShaderDataType::Sampler2D:   return 1;
				case ShaderDataType::SamplerCube: return 1;
			
				SAT_CORE_ASSERT( false, "Unknown ShaderDataType!" );
			}
		}
	};

	class VertexBufferLayout
	{
	public:
		VertexBufferLayout() = default;
		
		VertexBufferLayout( const std::initializer_list< VertexBufferElement >& elements )
			: m_Elements( elements )
		{
			uint32_t offset = 0;
			m_Stride = 0;
			for( auto& element : m_Elements )
			{
				element.Offset = offset;
				offset += element.Size;
				m_Stride += element.Size;
			}
		}

		VertexBufferLayout( const VertexBufferLayout& other )
			: m_Elements( other.m_Elements ), m_Stride( other.m_Stride )
		{
		}
		
		/*
		constexpr VertexBufferLayout& operator=( VertexBufferLayout&& rrOther ) noexcept
		{
			if( this == &rrOther )
				return *this;
			
			m_Elements = std::move( rrOther.m_Elements );
			m_Stride = rrOther.m_Stride;
			
			return *this;
		}
		
		constexpr VertexBufferLayout& operator=( const VertexBufferLayout& rOther ) noexcept
		{
			if( this == &rOther )
				return *this;
			
			m_Elements = rOther.m_Elements;
			m_Stride = rOther.m_Stride;
			
			return *this;
		}
		*/

		uint32_t Count() const { return (uint32_t)m_Elements.size(); }

		inline const std::vector< VertexBufferElement >& GetElements() const { return m_Elements; }
		inline uint32_t GetStride() const { return m_Stride; }
		
	public:
		// Usage for itr based for loops
		[[nodiscard]] std::vector< VertexBufferElement >::iterator begin() { return m_Elements.begin(); }
		[[nodiscard]] std::vector< VertexBufferElement >::iterator end() { return m_Elements.end(); }
		[[nodiscard]] std::vector< VertexBufferElement >::const_iterator begin() const { return m_Elements.begin(); }
		[[nodiscard]] std::vector< VertexBufferElement >::const_iterator end() const { return m_Elements.end(); }

	private:
		std::vector< VertexBufferElement> m_Elements;
		uint32_t m_Stride = 0;
	};
	
	// A vulkan vertex buffer.
	class VertexBuffer : public RefTarget
	{
	public:
		VertexBuffer() : m_pData( nullptr ) { }
		VertexBuffer( void* pData, VkDeviceSize Size, VkBufferUsageFlags Usage = 0 );
		VertexBuffer( VkDeviceSize Size, VkBufferUsageFlags Usage = 0 );
		VertexBuffer( const VertexBuffer& ) = delete;
		~VertexBuffer();

		void Destroy();
	
		void Bind( VkCommandBuffer CommandBuffer, uint32_t binding = 0 );
		void Bind( VkCommandBuffer CommandBuffer, uint32_t binding, VkDeviceSize* Offsets );

		void SetData( void* pData, uint32_t size, uint32_t offset = 0 );

		void Draw( VkCommandBuffer CommandBuffer );
		void BindAndDraw( VkCommandBuffer CommandBuffer );

#if !defined(SAT_DIST)
		void SetDebugName( const std::string& rName );
#endif

	private:
		void CreateBuffer();
	private:
		
		void* m_pData = nullptr;
		size_t m_Size = 0;
	
		VkBuffer m_Buffer = nullptr;

		VmaAllocation m_Allocation = nullptr;
	};
}