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

	static Ref<PhysicsMaterialAsset> GetPhysicsMaterial( Ref<StaticMesh> mesh )
	{
		Ref<PhysicsMaterialAsset> materialAsset;

		Ref<Project> activeProject = Project::GetActiveProject();
		if( mesh->GetPhysicsMaterial() == 0 || mesh->GetPhysicsMaterial() == activeProject->GetDefaultPhysicsMaterialAsset() )
		{
			materialAsset = AssetManager::Get()->GetAssetAs<PhysicsMaterialAsset>( activeProject->GetDefaultPhysicsMaterialAsset() );
		}
		else
		{
			materialAsset = AssetManager::Get()->GetAssetAs<PhysicsMaterialAsset>( mesh->GetPhysicsMaterial() );
		}

		return materialAsset;
	}

	PhysicsCooking::PhysicsCooking()
	{
	}

	void PhysicsCooking::Init()
	{
	}

	void PhysicsCooking::Terminate()
	{
	}

	PhysicsCooking::~PhysicsCooking()
	{
	}

	bool PhysicsCooking::CookMeshCollider( const Ref<StaticMesh>& rMesh, PhysicsShapeType Type )
	{
		if( Type <= PhysicsShapeType::Capusle )
			return false;

		bool Result = false;

		switch( Type )
		{
			case Saturn::PhysicsShapeType::ConvexMesh: 
			{
				Result = TryCookConvexMesh( rMesh );
			} break;

			case Saturn::PhysicsShapeType::TriangleMesh:
			{
				Result = TryCookTriangleMesh( rMesh );
			} break;

			case Saturn::PhysicsShapeType::Unknown:
			case Saturn::PhysicsShapeType::Box:
			case Saturn::PhysicsShapeType::Sphere:
			case Saturn::PhysicsShapeType::Capusle:
			default:
				break;
		}

		WriteCache( rMesh, Type );
		ClearCache();

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
			return nullptr;

		std::filesystem::path cachePath = Project::GetActiveProject()->GetFullCachePath();
		cachePath /= mesh->Name;
		cachePath.replace_extension( ".smcs" );

		if( !LoadColliderFile( cachePath ) )
			return nullptr;
		
		TransformComponent worldTC = entity->GetScene()->GetWorldSpaceTransform( entity );

		const auto& rVertices = mesh->Vertices();
		const auto& rIndices = mesh->Indices();
		const auto& rSubmeshes = mesh->Submeshes();

		JPH::StaticCompoundShapeSettings compoundShapeSettings;

		size_t index = 0llu;
		for( const auto& rCookedData : m_SubmeshData )
		{
			const Submesh& rSubmesh = mesh->Submeshes()[ index ];

			glm::vec3 submeshPosition, submeshScale;
			glm::quat submeshRotation;

			Maths::DecomposeTransform( rSubmesh.Transform, submeshPosition, submeshRotation, submeshScale );

			JoltBinaryReader reader( rCookedData.Stream );
			JPH::Shape::ShapeResult result = JPH::Shape::sRestoreFromBinaryState( reader );

			if( result.HasError() )
			{
				SAT_CORE_ERROR( "[JoltPhys]: Unable to create submesh shape for static compound shape! Index/{0}", index );
				return nullptr;
			}

			compoundShapeSettings.AddShape( 
				Auxiliary::GLMToJolt( submeshPosition ), 
				Auxiliary::GLMQToJoltQ( submeshRotation ), 
				new JPH::ScaledShape( result.Get(), Auxiliary::GLMToJolt( submeshScale * worldTC.Scale ) )
			);

			++index;
		}

		JPH::Shape::ShapeResult result = compoundShapeSettings.Create();

		if( result.HasError() )
		{
			SAT_CORE_ERROR( "[JoltPhys]: Unable to create static compound shape! Index/{0}, Error: {1}", index, result.GetError() );
		}

		return result.Get();
	}

	bool PhysicsCooking::TryCookTriangleMesh( const Ref<StaticMesh>& rMesh )
	{
		bool Result = false;

		const auto& rVertices = rMesh->Vertices();
		const auto& rIndices = rMesh->Indices();
		const auto& rSubmeshes = rMesh->Submeshes();

		size_t index = 0;
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

				Result = false;
				break;
			}

			JPH::RefConst<JPH::Shape> shape = res.Get();

			JoltBinaryWriter writer;
			shape->SaveBinaryState( writer );

			SubmeshColliderData& rData = m_SubmeshData.emplace_back();
			rData.Index = index;
			rData.Stream = writer.ToBuffer();

			++index;

			Result = true;
		}

		return Result;
	}

	bool PhysicsCooking::TryCookConvexMesh( const Ref<StaticMesh>& rMesh )
	{
		bool Result = false;
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

		for( uint32_t i = 0; i < hd.Submeshes; i++ )
		{
			SubmeshColliderData submesh{};

			RawSerialisation::ReadObject( submesh.Index, stream );
			RawSerialisation::ReadSaturnBuffer( submesh.Stream, stream );

			m_SubmeshData.push_back( submesh );
		}

		stream.close();
		return true;
	}

	void PhysicsCooking::WriteCache( const Ref<StaticMesh>& rMesh, PhysicsShapeType Type )
	{
		std::filesystem::path cachePath = Project::GetActiveProject()->GetFullCachePath();

		if( !std::filesystem::exists( cachePath ) )
			std::filesystem::create_directories( cachePath );

		cachePath /= rMesh->Name;
		cachePath.replace_extension(".smcs" );

		MeshCacheHeader hd{};
		hd.Type = Type;
		hd.ID = rMesh->ID;
		hd.Submeshes = rMesh->Submeshes().size();

		std::ofstream fout( cachePath, std::ios::binary | std::ios::trunc );

		RawSerialisation::WriteObject( hd, fout );

		for( auto& rMeshData : m_SubmeshData )
		{
			RawSerialisation::WriteObject( rMeshData.Index, fout );
			RawSerialisation::WriteSaturnBuffer( rMeshData.Stream, fout );
		}

		fout.close();
	}

	void PhysicsCooking::ClearCache()
	{
		m_SubmeshData.clear();
	}
}
