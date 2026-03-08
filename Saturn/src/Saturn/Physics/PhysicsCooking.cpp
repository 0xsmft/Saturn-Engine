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

#include "Saturn/Asset/AssetManager.h"

#include "Saturn/Project/Project.h"

#include "Saturn/Core/Maths.h"

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

	bool PhysicsCooking::TryCookTriangleMesh( const Ref<StaticMesh>& rMesh )
	{
		bool Result = false;
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

		Buffer fileBuffer;
		std::ifstream stream( rPath, std::ios::binary | std::ios::ate );

		auto end = stream.tellg();
		stream.seekg( 0, std::ios::beg );
		auto size = end - stream.tellg();

		fileBuffer.Allocate( ( size_t ) size );
		stream.read( reinterpret_cast< char* >( fileBuffer.Data ), fileBuffer.Size );

		stream.close();

		MeshCacheHeader hd = *( MeshCacheHeader* ) fileBuffer.Data;

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

		uint8_t* colliderData = fileBuffer.As<uint8_t>() + sizeof( MeshCacheHeader );

		for( uint32_t i = 0; i < hd.Submeshes; i++ )
		{
			SubmeshColliderData submesh{};

			uint32_t index = *( uint32_t* ) colliderData;

			colliderData += sizeof( uint32_t );

			size_t size = *( size_t* ) colliderData;

			colliderData += sizeof( size_t );

			submesh.Index = index;
			submesh.Stream = Buffer::Copy( colliderData, size );

			m_SubmeshData.push_back( submesh );

			colliderData += size;
		}

		fileBuffer.Free();

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
		hd.ID = rMesh->ID;
		hd.Submeshes = rMesh->Submeshes().size();
		hd.Type = Type;

		std::ofstream fout( cachePath, std::ios::binary | std::ios::trunc );

		fout.write( reinterpret_cast< char* >( &hd ), sizeof( MeshCacheHeader ) );

		for( auto& rMeshData : m_SubmeshData )
		{
			fout.write( reinterpret_cast<char*>( &rMeshData.Index ), sizeof( uint32_t ) );

			fout.write( reinterpret_cast< char* >( &rMeshData.Stream.Size ), sizeof( size_t ) );

			fout.write( reinterpret_cast< char* >( rMeshData.Stream.Data ), rMeshData.Stream.Size );
		}
	}

	void PhysicsCooking::ClearCache()
	{
		m_SubmeshData.clear();
	}
}
