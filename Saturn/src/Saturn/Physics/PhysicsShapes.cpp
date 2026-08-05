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
#include "PhysicsShapes.h"

#include "PhysicsAuxiliary.h"
#include "PhysicsFoundation.h"
#include "PhysicsMaterialAsset.h"

#include "Saturn/Asset/AssetManager.h"

#include "Saturn/Project/Project.h"

#include "Saturn/Scene/Scene.h"

#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>

namespace Saturn {

	//////////////////////////////////////////////////////////////////////////

	Ref<PhysicsMaterialAsset> PhysicsShape::GetMaterial( Ref<StaticMesh> mesh, UUID physMaterialAssetID )
	{
		Ref<PhysicsMaterialAsset> materialAsset;

		Ref<Project> activeProject = Project::GetActiveProject();
		const UUID fallbackID = activeProject->GetDefaultPhysicsMaterialAsset();

		// The PhysMat ID from the rigidbody takes priority over the mesh and the project fallback.
		if( physMaterialAssetID )
		{
			materialAsset = AssetManager::Get()->GetAssetAs<PhysicsMaterialAsset>( physMaterialAssetID );
		}

		// Still null then we go to the mesh and see what its material is.
		if( !materialAsset )
		{
			// Check if no mesh?
			if( !mesh )
			{
				// No mesh? Use prj default
				if( fallbackID != 0 )
				{
					materialAsset = AssetManager::Get()->GetAssetAs<PhysicsMaterialAsset>( fallbackID );
				}
			}
			// Hooray, we have a mesh
			// However, we must make sure that it's ID is not zero
			else if( mesh->GetPhysicsMaterial() == 0 || mesh->GetPhysicsMaterial() == fallbackID )
			{
				materialAsset = AssetManager::Get()->GetAssetAs<PhysicsMaterialAsset>( fallbackID );
			}
			// We have a mesh and it has a unique ID
			else
			{
				materialAsset = AssetManager::Get()->GetAssetAs<PhysicsMaterialAsset>( mesh->GetPhysicsMaterial() );
			}

			// If we get here, then we had no mesh and no fallback ID OR the specified ID could not be found.
			// So create memory only asset
			if( !materialAsset )
			{
				materialAsset = Ref<PhysicsMaterialAsset>::Create( 1.0f, 0.5f );
			}
		}

		SAT_CORE_ASSERT( materialAsset, "Material cannot be null at this stage!, All possible ways have failed, Rigidbody PhysMat could not be loaded and/or mesh PhysMat could not be loaded and/or project fallback could not be loaded and somehow the memeory only asset has failed as well." );

		return materialAsset;
	}

	//////////////////////////////////////////////////////////////////////////
	// Box

	BoxShape::BoxShape( SharedPtr<Entity> entity )
		: PhysicsShape( entity )
	{
		m_Type = PhysicsShapeType::Box;
	}

	BoxShape::~BoxShape()
	{
	}

	void BoxShape::Create( float mass )
	{
		BoxColliderComponent& bcc = m_Entity->GetComponent<BoxColliderComponent>();
		TransformComponent& transform = m_Entity->GetComponent<TransformComponent>();
		
		// Check for rigid body
		UUID rigidBodyMaterialID = 0;
		if( m_Entity->HasComponent<RigidbodyComponent>() )
			rigidBodyMaterialID = m_Entity->GetComponent<RigidbodyComponent>().MaterialAssetID;

		Ref<StaticMesh> mesh = nullptr;
		if( auto* pSM = m_Entity->TryGetComponent<StaticMeshComponent>(); pSM )
			mesh = pSM->Mesh;

		glm::vec3 halfSize = bcc.HalfExtents;

		// Very rare path, only happens if something else modifies the scale.
		if( bcc.AutoAdjustExtent && halfSize != transform.Scale )
			halfSize = transform.Scale * 0.5f;

		Ref<PhysicsMaterialAsset> materialAsset = GetMaterial( mesh, rigidBodyMaterialID );

		const glm::vec3 halfColliderSize = bcc.AutoAdjustExtent ? halfSize : glm::abs( transform.Scale * halfSize );
	
		// v = lbh
		const float volume = halfColliderSize.x * 2.0f * halfColliderSize.y * 2.0f * halfColliderSize.z * 2.0f;

		JPH::BoxShapeSettings shapeSettings( Auxiliary::GLMToJolt( halfColliderSize ) );
		// d = m/v
		shapeSettings.mDensity = mass / volume;
		shapeSettings.mMaterial = materialAsset->GetNative();

		m_Shape = shapeSettings.Create().Get();
	}

	void BoxShape::SetTrigger( bool isTrigger )
	{
		m_Entity->GetComponent<BoxColliderComponent>().IsTrigger = isTrigger;
	}

	bool BoxShape::IsTrigger()
	{
		return m_Entity->GetComponent<BoxColliderComponent>().IsTrigger;
	}

	//////////////////////////////////////////////////////////////////////////
	// Sphere

	SphereShape::SphereShape( SharedPtr<Entity> entity )
		: PhysicsShape( entity )
	{
		m_Type = PhysicsShapeType::Sphere;
	}

	SphereShape::~SphereShape()
	{
	}

	void SphereShape::Create( float mass )
	{
		SphereColliderComponent& scc = m_Entity->GetComponent<SphereColliderComponent>();
		TransformComponent& transform = m_Entity->GetComponent<TransformComponent>();

		// Check for rigid body
		UUID rigidBodyMaterialID = 0;
		if( m_Entity->HasComponent<RigidbodyComponent>() )
			rigidBodyMaterialID = m_Entity->GetComponent<RigidbodyComponent>().MaterialAssetID;

		Ref<StaticMesh> mesh = nullptr;
		if( auto* pSM = m_Entity->TryGetComponent<StaticMeshComponent>(); pSM )
			mesh = pSM->Mesh;

		float radius = scc.Radius;
		glm::vec3 scale = transform.Scale;

		if( scale.x != 0.0f )
			radius *= scale.x;

		Ref<PhysicsMaterialAsset> materialAsset = GetMaterial( mesh, rigidBodyMaterialID );
		
		const float largestComponent = glm::abs( glm::max( transform.Scale.x, glm::max( transform.Scale.y, transform.Scale.z ) ) );
		const float scaledRadius = largestComponent * radius;
		const float volume = ( 4.0f / 3.0f ) * glm::pi<float>() * ( float ) glm::pow( scaledRadius, 3.0f );

		JPH::SphereShapeSettings sphereSetting( radius, nullptr );
		sphereSetting.mDensity = mass / volume;
		sphereSetting.mMaterial = materialAsset->GetNative();

		m_Shape = sphereSetting.Create().Get();
	}

	void SphereShape::SetTrigger( bool isTrigger )
	{
		m_Entity->GetComponent<SphereColliderComponent>().IsTrigger = isTrigger;
	}

	bool SphereShape::IsTrigger()
	{
		return m_Entity->GetComponent<SphereColliderComponent>().IsTrigger;
	}

	//////////////////////////////////////////////////////////////////////////
	// Capsule

	CapsuleShape::CapsuleShape( SharedPtr<Entity> entity )
		: PhysicsShape( entity )
	{
		m_Type = PhysicsShapeType::Capusle;
	}

	CapsuleShape::~CapsuleShape()
	{
	}

	void CapsuleShape::Create( float mass )
	{
		CapsuleColliderComponent& cap = m_Entity->GetComponent<CapsuleColliderComponent>();
		TransformComponent& transform = m_Entity->GetComponent<TransformComponent>();

		// Check for rigid body
		UUID rigidBodyMaterialID = 0;
		if( m_Entity->HasComponent<RigidbodyComponent>() )
			rigidBodyMaterialID = m_Entity->GetComponent<RigidbodyComponent>().MaterialAssetID;

		// Check for static mesh
		Ref<StaticMesh> mesh = nullptr;
		if( auto* pSM = m_Entity->TryGetComponent<StaticMeshComponent>(); pSM )
			mesh = pSM->Mesh;

		float radius = cap.Radius;
		float height = cap.HalfHeight;

		glm::vec3 scale = transform.Scale;

		if( scale.x != 0.0f && height == 0.0f )
			radius *= scale.x;

		if( scale.y != 0.0f && height == 0.0f )
			height *= scale.y;

		Ref<PhysicsMaterialAsset> materialAsset = GetMaterial( mesh, rigidBodyMaterialID );
		
		JPH::CapsuleShapeSettings capsuleSetting( height, radius, nullptr );
		capsuleSetting.mDensity = 10.0f;
		capsuleSetting.mMaterial = materialAsset->GetNative();

		m_Shape = capsuleSetting.Create().Get();
	}

	void CapsuleShape::SetTrigger( bool isTrigger )
	{
		m_Entity->GetComponent<CapsuleColliderComponent>().IsTrigger = isTrigger;
	}

	bool CapsuleShape::IsTrigger()
	{
		return m_Entity->GetComponent<CapsuleColliderComponent>().IsTrigger;
	}

	//////////////////////////////////////////////////////////////////////////
	// Triangle

	TriangleMeshShape::TriangleMeshShape( SharedPtr<Entity> entity )
		: PhysicsShape( entity )
	{
		m_Type = PhysicsShapeType::TriangleMesh;

		SAT_CORE_ASSERT( m_Entity->HasComponent<StaticMeshComponent>(), "Entity does not have a static mesh component!" );
	}

	TriangleMeshShape::~TriangleMeshShape()
	{
	}

	void TriangleMeshShape::Create( float mass )
	{
		Ref<StaticMesh> staticMesh = m_Entity->GetComponent<StaticMeshComponent>().Mesh;
		m_Shape = PhysicsFoundation::Get()->GetCooking().CreateTriangleMesh( m_Entity, staticMesh );
	}

	bool TriangleMeshShape::IsTrigger()
	{
		// TODO: Mesh colliders as triggers
		return false;
	}

	//////////////////////////////////////////////////////////////////////////
	// Convex

	ConvexMeshShape::ConvexMeshShape( SharedPtr<Entity> entity )
		: PhysicsShape( entity )
	{
		m_Type = PhysicsShapeType::ConvexMesh;

		SAT_CORE_ASSERT( m_Entity->HasComponent<StaticMeshComponent>(), "Entity does not have a static mesh component!" );
	}

	ConvexMeshShape::~ConvexMeshShape()
	{
	}

	void ConvexMeshShape::Create( float mass )
	{
		Ref<StaticMesh> staticMesh = m_Entity->GetComponent<StaticMeshComponent>().Mesh;
		m_Shape = PhysicsFoundation::Get()->GetCooking().CreateConvexMesh( m_Entity, staticMesh );
	}

	bool ConvexMeshShape::IsTrigger()
	{
		// TODO: Mesh colliders as triggers
		return false;
	}

}
