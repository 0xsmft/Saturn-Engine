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

#include "Saturn/Vulkan/Mesh.h"
#include "Saturn/Scene/Entity.h"

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Collision/Shape/Shape.h>

namespace Saturn {

	// File header
	struct MeshCacheHeader
	{
		// .SMC / SMCS
		const unsigned char Magic[ 4 ] = { 0x53, 0x4D, 0x43, 0x00 };
		PhysicsShapeType Type;
		uint64_t ID = 0ull;
		size_t Submeshes = 0ull;
	};

	// Data for each submesh
	struct SubmeshColliderData
	{
		// Submesh Index in it's mesh
		uint32_t Index = UINT32_MAX;
		Buffer Stream;
	};

	enum class PhysicsCookingResult 
	{
		Success,
		InvalidTypeForCooking,
		Failure
	};

	class PhysicsCooking
	{
	public:
		PhysicsCooking();
		~PhysicsCooking();

		void Init();
		void Terminate();

	public:
		//
		// Cook the collider type, into a binary file. Overriding any existing cache file.
		//
		PhysicsCookingResult CookMeshCollider( const Ref<StaticMesh> mesh, PhysicsShapeType Type );

		JPH::Ref<JPH::Shape> CreateTriangleMesh( SharedPtr<Entity> entity, Ref<StaticMesh> mesh );
		JPH::Ref<JPH::Shape> CreateConvexMesh( SharedPtr<Entity> entity, Ref<StaticMesh> mesh );

	private:
		void ClearCache();
		void WriteCache( const Ref<StaticMesh> mesh, PhysicsShapeType Type );
		bool LoadColliderFile( const std::filesystem::path& rPath );
		bool MeshColliderAlreadyLoaded( const Ref<StaticMesh> mesh );

		PhysicsCookingResult TryCookTriangleMesh( const Ref<StaticMesh> mesh );
		PhysicsCookingResult TryCookConvexMesh( const Ref<StaticMesh> mesh );

	private:
		//					MESH ID -> PER SUBMESH DATA
		std::unordered_map<UUID, std::vector<SubmeshColliderData>> m_NewSubmeshData;
	};
}
