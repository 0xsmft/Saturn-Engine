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
#include "PhysicsCooking.h"

#include "PhysicsAuxiliary.h"
#include "PhysicsFoundation.h"
#include "PhysicsMaterialAsset.h"
#include "JoltBinaryHelpers.h"

#include "Saturn/Asset/AssetManager.h"

#include "Saturn/Project/Project.h"

#include "Saturn/Core/Maths.h"

#include "Saturn/Serialisation/Raw/RawSerialisation.h"

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Collision/Shape/MeshShape.h>
#include <Jolt/Physics/Collision/Shape/StaticCompoundShape.h>
#include <Jolt/Physics/Collision/Shape/ScaledShape.h>
#include <Jolt/Physics/Collision/Shape/ConvexHullShape.h>

namespace Saturn {

	PhysicsCooking::PhysicsCooking()
	{
	}

	void PhysicsCooking::Init()
	{
	}

	void PhysicsCooking::Terminate()
	{
		ClearCache();
	}

	PhysicsCooking::~PhysicsCooking()
	{
		Terminate();
	}

	PhysicsCookingResult PhysicsCooking::CookMeshCollider( const Ref<StaticMesh> mesh, PhysicsShapeType Type )
	{
		if( Type <= PhysicsShapeType::Capusle )
			return PhysicsCookingResult::InvalidTypeForCooking;

		PhysicsCookingResult Result = PhysicsCookingResult::Failure;
		switch( Type )
		{
			case Saturn::PhysicsShapeType::ConvexMesh: 
			{
				Result = TryCookConvexMesh( mesh );
			} break;

			case Saturn::PhysicsShapeType::TriangleMesh:
			{
				Result = TryCookTriangleMesh( mesh );
			} break;

			case Saturn::PhysicsShapeType::Unknown:
			case Saturn::PhysicsShapeType::Box:
			case Saturn::PhysicsShapeType::Sphere:
			case Saturn::PhysicsShapeType::Capusle:
			default:
				SAT_CORE_WARN( "Invalid or unhandled type specified into CookMeshCollider! Only Convex and Triangle meshs can be cooked!" );
				break;
		}

		// Write file
		WriteCache( mesh, Type );

		return Result;
	}

	template<typename T, typename U>
	JPH::Ref<T> CastJoltRef( const JPH::Ref<U> ref )
	{
		return JPH::Ref<T>( static_cast< T* >( const_cast< U* >( ref.GetPtr() ) ) );
	}

	JPH::Ref<JPH::Shape> PhysicsCooking::CreateTriangleMesh( SharedPtr<Entity> entity, Ref<StaticMesh> mesh )
	{
		if( !mesh ) 
		{
			SAT_CORE_ERROR( "[PhysicsCooking]: Mesh is null! Not creating a triangle mesh on a null mesh." );
			return nullptr;
		}

		std::filesystem::path cachePath = Project::GetActiveProject()->GetFullCachePath();
		cachePath /= std::to_string( mesh->ID );
		cachePath.replace_extension( ".smcs" );

		if( !MeshColliderAlreadyLoaded( mesh ) && !LoadColliderFile( cachePath ) )
		{
			SAT_CORE_ERROR( "[PhysicsCooking]: Unable to load collider file. Is it a valid Saturn collider cache?" );
			return nullptr;
		}
		
		TransformComponent worldTC = entity->GetScene()->GetWorldSpaceTransform( entity );

		const auto& rVertices = mesh->Vertices();
		const auto& rIndices = mesh->Indices();
		const auto& rSubmeshes = mesh->Submeshes();

		JPH::StaticCompoundShapeSettings compoundShapeSettings;

		size_t subMeshIndex = 0llu;
		for( const auto& rCookedData : m_NewSubmeshData[ mesh->ID ] )
		{
			const Submesh& rSubmesh = mesh->Submeshes()[ subMeshIndex ];

			glm::vec3 submeshPosition, submeshScale;
			glm::quat submeshRotation;

			Maths::DecomposeTransform( rSubmesh.Transform, submeshPosition, submeshRotation, submeshScale );

			JoltBinaryReader reader( rCookedData.Stream );
			JPH::Shape::ShapeResult result = JPH::Shape::sRestoreFromBinaryState( reader );

			if( result.HasError() )
			{
				SAT_CORE_ERROR( "[JoltPhys]: Unable to create submesh shape for static compound shape! Index/{0}", subMeshIndex );
				return nullptr;
			}

			compoundShapeSettings.AddShape( 
				Auxiliary::GLMToJolt( submeshPosition ), 
				Auxiliary::GLMQToJoltQ( submeshRotation ), 
				new JPH::ScaledShape( result.Get(), Auxiliary::GLMToJolt( submeshScale * worldTC.Scale ) )
			);

			++subMeshIndex;
		}

		JPH::Shape::ShapeResult result = compoundShapeSettings.Create();

		if( result.HasError() )
		{
			SAT_CORE_ERROR( "[JoltPhys]: Unable to create static compound shape! Index/{0}, Error: {1}", subMeshIndex, result.GetError() );
		}

		return result.Get();
	}

	JPH::Ref<JPH::Shape> PhysicsCooking::CreateConvexMesh( SharedPtr<Entity> entity, Ref<StaticMesh> mesh )
	{
		if( !mesh )
		{
			SAT_CORE_ERROR( "[PhysicsCooking]: Mesh is null! Not creating a convex mesh on a null mesh." );
			return nullptr;
		}

		std::filesystem::path cachePath = Project::GetActiveProject()->GetFullCachePath();
		cachePath /= std::to_string( mesh->ID );
		cachePath.replace_extension( ".smcs" );

		if( !MeshColliderAlreadyLoaded( mesh ) && !LoadColliderFile( cachePath ) )
		{
			SAT_CORE_ERROR( "[PhysicsCooking]: Unable to load collider file. Is it a valid Saturn collider cache?" );
			return nullptr;
		}

		TransformComponent worldTC = entity->GetScene()->GetWorldSpaceTransform( entity );

		const auto& rVertices = mesh->Vertices();
		const auto& rIndices = mesh->Indices();
		const auto& rSubmeshes = mesh->Submeshes();
		const Submesh& rSubmesh = mesh->Submeshes()[ 0 ];
		const auto& rCookedData = m_NewSubmeshData[ mesh->ID ][ 0 ];

		glm::vec3 submeshPosition, submeshScale;
		glm::quat submeshRotation;

		Maths::DecomposeTransform( rSubmesh.Transform, submeshPosition, submeshRotation, submeshScale );

		JoltBinaryReader reader( rCookedData.Stream );
		JPH::Shape::ShapeResult result = JPH::Shape::sRestoreFromBinaryState( reader );

		if( result.HasError() )
		{
			SAT_CORE_ERROR( "[JoltPhys]: Unable to create shape for convex mesh!" );
			return nullptr;
		}

		JPH::Ref<JPH::ScaledShape> scaledConvexMesh = new JPH::ScaledShape( result.Get(), Auxiliary::GLMToJolt( submeshScale * worldTC.Scale ) );

		return ( JPH::Ref<JPH::Shape> )scaledConvexMesh;
	}

	PhysicsCookingResult PhysicsCooking::TryCookTriangleMesh( const Ref<StaticMesh> mesh )
	{
		PhysicsCookingResult Result = PhysicsCookingResult::Failure;

		const auto& rVertices = mesh->Vertices();
		const auto& rIndices = mesh->Indices();
		const auto& rSubmeshes = mesh->Submeshes();

		// Clear any existing cache.
		const auto itr = m_NewSubmeshData.find( mesh->ID );
		if( itr != m_NewSubmeshData.end() )
		{
			m_NewSubmeshData.erase( itr );
		}

		auto& rPerSubmeshData = m_NewSubmeshData[ mesh->ID ];

		// Now cook each submesh...
		size_t subMeshIndex = 0;
		for( const auto& rSubmesh : rSubmeshes )
		{
			JPH::VertexList vertList;
			JPH::IndexedTriangleList triList;

			for( uint32_t i = rSubmesh.BaseVertex; i < rSubmesh.BaseVertex + rSubmesh.VertexCount; ++i )
			{
				const auto& rVertex = rVertices[ i ];
				vertList.push_back( JPH::Float3( rVertex.Position.x, rVertex.Position.y, rVertex.Position.z ) );
			}

			for( uint32_t i = rSubmesh.BaseIndex / 3; i < rSubmesh.BaseIndex / 3 + rSubmesh.IndexCount / 3; ++i )
			{
				const auto& rIndex = rIndices[ i ];
				triList.push_back( JPH::IndexedTriangle( rIndex.V1, rIndex.V2, rIndex.V3, 0 ) );
			}

			JPH::RefConst<JPH::MeshShapeSettings> meshSettings = new JPH::MeshShapeSettings( vertList, triList );

			const auto res = meshSettings->Create();
			if( res.HasError() )
			{
				SAT_CORE_ERROR( "[JoltPhys]: Error: {0}", res.GetError() );

				Result = PhysicsCookingResult::Failure;
				break;
			}

			JPH::RefConst<JPH::Shape> shape = res.Get();

			JoltBinaryWriter writer;
			shape->SaveBinaryState( writer );

			SubmeshColliderData& rData = rPerSubmeshData.emplace_back();
			rData.Index = ( uint32_t ) subMeshIndex;
			rData.Stream = writer.ToBuffer();

			++subMeshIndex;

			Result = PhysicsCookingResult::Success;
		}

		return Result;
	}

	PhysicsCookingResult PhysicsCooking::TryCookConvexMesh( const Ref<StaticMesh> mesh )
	{
		PhysicsCookingResult Result = PhysicsCookingResult::Failure;

		const auto& rVertices = mesh->Vertices();
		const auto& rIndices = mesh->Indices();
		const auto& rSubmeshes = mesh->Submeshes();

		// Clear any existing cache.
		const auto itr = m_NewSubmeshData.find( mesh->ID );
		if( itr != m_NewSubmeshData.end() )
		{
			m_NewSubmeshData.erase( itr );
		}

		auto& rPerSubmeshData = m_NewSubmeshData[ mesh->ID ];

		// Now cook the submeshes...
		size_t subMeshIndex = 0;
		for( const auto& rSubmesh : rSubmeshes )
		{
			// Convex mesh needs at least 3+ vertices
			if( rSubmesh.VertexCount < 3 )
			{
				rPerSubmeshData.emplace_back();
				continue;
			}

			JPH::Array<JPH::Vec3> positions;

			for( uint32_t i = rSubmesh.BaseVertex / 3; i < ( rSubmesh.BaseVertex / 3 ) + ( rSubmesh.VertexCount / 3 ); ++i )
			{
				const Index& rIndex = rIndices[ i ];
				const StaticVertex& v1 = rVertices[ rIndex.V1 ];
				const StaticVertex& v2 = rVertices[ rIndex.V2 ];
				const StaticVertex& v3 = rVertices[ rIndex.V3 ];

				positions.push_back( JPH::Vec3( v1.Position.x, v1.Position.y, v1.Position.z ) );
				positions.push_back( JPH::Vec3( v2.Position.x, v2.Position.y, v2.Position.z ) );
				positions.push_back( JPH::Vec3( v3.Position.x, v3.Position.y, v3.Position.z ) );
			}

			JPH::RefConst<JPH::ConvexHullShapeSettings> meshSettings = new JPH::ConvexHullShapeSettings( positions );

			const auto res = meshSettings->Create();
			if( res.HasError() )
			{
				SAT_CORE_ERROR( "[JoltPhys]: Error: {0}", res.GetError() );

				Result = PhysicsCookingResult::Failure;
				break;
			}

			JPH::RefConst<JPH::Shape> shape = res.Get();

			JoltBinaryWriter writer;
			shape->SaveBinaryState( writer );

			SubmeshColliderData& rData = rPerSubmeshData.emplace_back();
			rData.Index = ( uint32_t ) subMeshIndex;
			rData.Stream = writer.ToBuffer();

			++subMeshIndex;

			Result = PhysicsCookingResult::Success;
		}

		return Result;
	}

	bool PhysicsCooking::LoadColliderFile( const std::filesystem::path& rPath )
	{
		if( !std::filesystem::exists( rPath ) )
			return false;

		std::ifstream stream( rPath, std::ios::binary | std::ios::in );

		MeshCacheHeader hd{};
		RawSerialisation::ReadObject( hd, stream );

		if( std::memcmp( hd.Magic, "SMC", 4 ) != 0 )
		{
			SAT_CORE_ASSERT( false, "Invalid file header!" );
			return false;
		}

		if( hd.Type <= PhysicsShapeType::Capusle )
		{
			SAT_CORE_ASSERT( false, "Invalid mesh collider type! Must be TriangleMesh or ConvexMesh." );
			return false;
		}

		auto& rPerSubmeshData = m_NewSubmeshData[ hd.ID ];

		for( size_t i = 0; i < hd.Submeshes; ++i )
		{
			SubmeshColliderData submesh{};

			RawSerialisation::ReadObject( submesh.Index, stream );
			RawSerialisation::ReadSaturnBuffer( submesh.Stream, stream );

			rPerSubmeshData.push_back( submesh );
		}

		stream.close();
		return true;
	}

	bool PhysicsCooking::MeshColliderAlreadyLoaded( const Ref<StaticMesh> mesh )
	{
		return m_NewSubmeshData.find( mesh->ID ) != m_NewSubmeshData.end();
	}

	void PhysicsCooking::WriteCache( const Ref<StaticMesh> mesh, PhysicsShapeType Type )
	{
		// Before anything, check if we even exist in the cache...
		const auto itr = m_NewSubmeshData.find( mesh->ID );
		if( itr == m_NewSubmeshData.end() )
		{
			return;
		}

		// ... if so we can write.
		std::filesystem::path cachePath = Project::GetActiveProject()->GetFullCachePath();

		if( !std::filesystem::exists( cachePath ) )
			std::filesystem::create_directories( cachePath );

		cachePath /= std::to_string( mesh->ID );
		cachePath.replace_extension(".smcs" );

		auto& rSubmeshData = itr->second;

		MeshCacheHeader hd{};
		hd.Type = Type;
		hd.ID = mesh->ID;
		hd.Submeshes = mesh->Submeshes().size();

		std::ofstream fout( cachePath, std::ios::binary | std::ios::trunc );

		RawSerialisation::WriteObject( hd, fout );

		for( auto& rMeshData : rSubmeshData )
		{
			RawSerialisation::WriteObject( rMeshData.Index, fout );
			RawSerialisation::WriteSaturnBuffer( rMeshData.Stream, fout );
		}

		fout.close();
	}

	void PhysicsCooking::ClearCache()
	{
		for( auto& [id, perSubmeshData] : m_NewSubmeshData )
		{
			for( auto& rSubmeshData : perSubmeshData )
			{
				// Make sure we clear the data to avoid a leak!
				rSubmeshData.Stream.Free();
			}
		}

		m_NewSubmeshData.clear();
	}

}
