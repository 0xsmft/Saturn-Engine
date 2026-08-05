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

#include "Saturn/Core/Base.h"
#include "Saturn/Scene/Entity.h"

#include "PhysicsShapeTypes.h"

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Collision/Shape/Shape.h>

namespace Saturn {

	class PhysicsMaterialAsset;
	struct RecastInputGeometryExpData;

	class PhysicsShape : public RefTarget
	{
	public:
		PhysicsShape( SharedPtr<Entity> entity ) { m_Entity = entity; }
		virtual ~PhysicsShape() = default;

	public:
		virtual void Create( float mass ) = 0;

	public:
		// Runtime only!
		virtual void SetTrigger( bool isTrigger ) = 0;
		[[nodiscard]] virtual bool IsTrigger() = 0;

	public:
		JPH::ShapeRefC GetShape() const { return m_Shape; }
		PhysicsShapeType GetType() const { return m_Type; }

	protected:
		Ref<PhysicsMaterialAsset> GetMaterial( Ref<StaticMesh> mesh, UUID physMaterialAssetID );

	protected:
		SharedPtr<Entity> m_Entity;
		JPH::Ref<JPH::Shape> m_Shape = nullptr;
		PhysicsShapeType m_Type = PhysicsShapeType::Unknown;
	};

	class BoxShape : public PhysicsShape
	{
	public:
		BoxShape( SharedPtr<Entity> entity );
		virtual ~BoxShape();

	public:
		void Create( float mass ) override;

	public:
		// Runtime only!
		virtual void SetTrigger( bool isTrigger ) override;
		[[nodiscard]] virtual bool IsTrigger() override;
	};

	class SphereShape : public PhysicsShape
	{
	public:
		SphereShape( SharedPtr<Entity> entity );
		virtual ~SphereShape();

	public:
		void Create( float mass ) override;
	
	public:
		// Runtime only!
		virtual void SetTrigger( bool isTrigger ) override;
		[[nodiscard]] virtual bool IsTrigger() override;
	};

	class CapsuleShape : public PhysicsShape
	{
	public:
		CapsuleShape( SharedPtr<Entity> entity );
		virtual ~CapsuleShape();

	public:
		void Create( float mass ) override;

	public:
		// Runtime only!
		virtual void SetTrigger( bool isTrigger ) override;
		[[nodiscard]] virtual bool IsTrigger() override;
	};

	class TriangleMeshShape : public PhysicsShape
	{
	public:
		TriangleMeshShape( SharedPtr<Entity> entity );
		virtual ~TriangleMeshShape();

	public:
		// This assumes the the mesh collider has already been cooked.
		virtual void Create( float mass ) override;

	public:
		virtual void SetTrigger( bool t ) { }
		[[nodiscard]] virtual bool IsTrigger() override;
	};

	//
	// NOTE: WIP!!
	//
	class ConvexMeshShape : public PhysicsShape
	{
	public:
		ConvexMeshShape( SharedPtr<Entity> entity );
		virtual ~ConvexMeshShape();

	public:
		// This assumes the the mesh collider has already been cooked.
		virtual void Create( float mass ) override;

	public:
		virtual void SetTrigger( bool t ) {}
		[[nodiscard]] virtual bool IsTrigger() override;
	};
}
