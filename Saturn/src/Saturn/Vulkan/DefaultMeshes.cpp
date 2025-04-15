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
#include "DefaultMeshes.h"

#include "Mesh.h"

constexpr auto M_PI = 3.14159265358979323846;

namespace Saturn::Auxiliary {

	Ref<StaticMesh> DefaultMeshes::CreateSphere( float radius )
	{
		std::vector<StaticVertex> vertices;
		std::vector<Index> indices;

		constexpr float latitudeBands = 30;
		constexpr float longitudeBands = 30;

		for( float latNumber = 0; latNumber <= latitudeBands; latNumber++ )
		{
			const float theta = latNumber * M_PI / latitudeBands;
			const float sinTheta = glm::sin( theta );
			const float cosTheta = glm::cos( theta );

			for( float longNumber = 0; longNumber <= longitudeBands; longNumber++ )
			{
				const float phi = longNumber * 2 * M_PI / longitudeBands;
				const float sinPhi = glm::sin( phi );
				const float cosPhi = glm::cos( phi );

				float x = cosPhi * sinTheta;
				float y = cosTheta;
				float z = sinPhi * sinTheta;

				StaticVertex vertex{};
				vertex.Position = { x * radius, y * radius, z * radius };
				vertex.Normal = { x, y, z };

				vertices.push_back( vertex );
			}
		}

		for( uint32_t latNumber = 0; latNumber < latitudeBands; latNumber++ )
		{
			for( uint32_t longNumber = 0; longNumber < longitudeBands; longNumber++ )
			{
				uint32_t first = ( latNumber * ( longitudeBands + 1 ) ) + longNumber;
				uint32_t second = first + longitudeBands + 1;

				indices.push_back( { first, second, first + 1 } );
				indices.push_back( { second, second + 1, first + 1 } );
			}
		}
		
		return Ref<StaticMesh>::Create( vertices, indices, glm::mat4( 1.0f ) );
	}

	Ref<StaticMesh> DefaultMeshes::CreateCube( const glm::vec3& size )
	{
		std::vector<StaticVertex> vertices;
		vertices.resize( 8 );

		vertices[ 0 ].Position = { -size.x / 2.0f, -size.y / 2.0f,  size.z / 2.0f };
		vertices[ 1 ].Position = { size.x / 2.0f, -size.y / 2.0f,  size.z / 2.0f };
		vertices[ 2 ].Position = { size.x / 2.0f,  size.y / 2.0f,  size.z / 2.0f };
		vertices[ 3 ].Position = { -size.x / 2.0f,  size.y / 2.0f,  size.z / 2.0f };
		vertices[ 4 ].Position = { -size.x / 2.0f, -size.y / 2.0f, -size.z / 2.0f };
		vertices[ 5 ].Position = { size.x / 2.0f, -size.y / 2.0f, -size.z / 2.0f };
		vertices[ 6 ].Position = { size.x / 2.0f,  size.y / 2.0f, -size.z / 2.0f };
		vertices[ 7 ].Position = { -size.x / 2.0f,  size.y / 2.0f, -size.z / 2.0f };

		vertices[ 0 ].Normal = { -1.0f, -1.0f,  1.0f };
		vertices[ 1 ].Normal = { 1.0f, -1.0f,  1.0f };
		vertices[ 2 ].Normal = { 1.0f,  1.0f,  1.0f };
		vertices[ 3 ].Normal = { -1.0f,  1.0f,  1.0f };
		vertices[ 4 ].Normal = { -1.0f, -1.0f, -1.0f };
		vertices[ 5 ].Normal = { 1.0f, -1.0f, -1.0f };
		vertices[ 6 ].Normal = { 1.0f,  1.0f, -1.0f };
		vertices[ 7 ].Normal = { -1.0f,  1.0f, -1.0f };

		std::vector<Index> indices = {
			{ 0, 1, 2 }, { 2, 3, 0 },
			{ 1, 5, 6 }, { 6, 2, 1 },
			{ 7, 6, 5 }, { 5, 4, 7 },
			{ 4, 0, 3 }, { 3, 7, 4 },
			{ 4, 5, 1 }, { 1, 0, 4 },
			{ 3, 2, 6 }, { 6, 7, 3 }
		};

		return Ref<StaticMesh>::Create( vertices, indices, glm::mat4( 1.0f ) );
	}

	static void CalcRing( size_t seg, float r, float y, float dy, float h, float actual, std::vector<StaticVertex>& rVertices ) 
	{
		float segIncr = 1.0f / ( float ) ( seg - 1 );
		for( size_t s = 0; s < seg; s++ )
		{
			float x = r * glm::cos( 2 * M_PI * s * segIncr );
			float z = r * glm::sin( 2 * M_PI * s * segIncr );
			
			StaticVertex& vertex = rVertices.emplace_back();
			vertex.Position = { x, y, z };
		}
	}

	Ref<StaticMesh> DefaultMeshes::CreateCapsule( float radius, float height )
	{
		constexpr size_t subdivisionHeight = 8;
		constexpr size_t ringsBody = subdivisionHeight + 1;
		constexpr size_t ringsTotal = subdivisionHeight + ringsBody;
		constexpr size_t numSegments = 12;
		constexpr size_t radiusMod = 0.021f;

		std::vector<StaticVertex> vertices;
		std::vector<Index> indices;

		vertices.reserve( numSegments * ringsTotal );
		indices.reserve( ( numSegments - 1 ) * ( ringsTotal - 1 ) * 2 );

		constexpr float bodyIncr = 1.0f / ( float ) ringsBody - 1;
		constexpr float ringIncr = 1.0f / ( float ) subdivisionHeight - 1;

		for( int r = 0; r < subdivisionHeight / 2; r++ )
		{
			float rad = glm::sin( float( M_PI ) * r * ringIncr );
			float y = glm::sin( float( M_PI ) * ( r * ringIncr - 0.5f ) );

			CalcRing( numSegments, rad, y, -0.5f, height, radius + radiusMod, vertices );
		}

		for( int r = 0; r < ringsBody; r++ )
		{
			float y = glm::sin( float( M_PI ) * ( r * ringIncr - 0.5f ) );

			CalcRing( numSegments, 1.0f, 0.0f, y, height, height + radiusMod, vertices );
		}

		for( int r = subdivisionHeight / 2; r < subdivisionHeight; r++ )
		{
			float rad = glm::sin( float( M_PI ) * r * ringIncr );
			float y = glm::sin( float( M_PI ) * ( r * ringIncr - 0.5f ) );

			CalcRing( numSegments, rad, y, 0.5f, height, rad + radiusMod, vertices );
		}

		for( int r = 0; r < ringsTotal - 1; r++ )
		{
			for( int s = 0; s < numSegments - 1; s++ )
			{
				Index& rIndex = indices.emplace_back();
				rIndex.V1 = ( uint32_t ) r * numSegments + s + 1;
				rIndex.V2 = ( uint32_t ) r * numSegments + s;
				rIndex.V3 = ( uint32_t ) ( r + 1 ) * numSegments + s + 1;

				Index& rIndex2 = indices.emplace_back();
				rIndex2.V1 = ( uint32_t ) ( r + 1 ) * numSegments + s;
				rIndex2.V2 = ( uint32_t ) ( r + 1 ) * numSegments + s + 1;
				rIndex2.V3 = ( uint32_t ) ( r * numSegments * s );
			}
		}

		return Ref<StaticMesh>::Create( vertices, indices, glm::mat4( 1.0f ) );
	}

}
