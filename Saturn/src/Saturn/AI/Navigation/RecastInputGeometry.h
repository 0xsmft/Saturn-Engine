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

#include "RecastChunkyTriMesh.h"

#include "Saturn/Vulkan/Mesh.h"
#include "Saturn/Core/Ref.h"

namespace Saturn {

	class RecastInputGeometry : public RefTarget
	{
	public:
		RecastInputGeometry() = default;
		~RecastInputGeometry() = default;

		void BeginImport();
		void Add( Ref<StaticMesh> mesh, const glm::mat4& rModel );
//		void Add( Ref<DynamicMesh> mesh );
		void EndImport();

		AABB GetAABB() const { return AABB( m_MinBounds, m_MaxBounds ); }
		const glm::vec3& GetMinBounds() const { return m_MinBounds; }
		const glm::vec3& GetMaxBounds() const { return m_MaxBounds; }

		rcChunkyTriMesh* GetChunkyTriMesh() const { return m_pChunkyTriMesh; }

		const std::vector<float>& GetVertexBuffer() const { return m_VertexBuffer; }
		const std::vector<int>& GetIndexBuffer() const { return m_IndexBuffer; }

		uint32_t GetFaceCount() const { return m_FaceCount; }

	private:
		std::vector<float> m_VertexBuffer;
		std::vector<int> m_IndexBuffer;

		glm::vec3 m_MinBounds{};
		glm::vec3 m_MaxBounds{};

		uint32_t m_FaceCount = 0;

	private:
		rcChunkyTriMesh* m_pChunkyTriMesh = nullptr;
	};
}