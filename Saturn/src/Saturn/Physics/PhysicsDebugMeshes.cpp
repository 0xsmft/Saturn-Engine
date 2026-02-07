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
#include "PhysicsDebugMeshes.h"

#include "Saturn/Vulkan/DefaultMeshes.h"

namespace Saturn {

	PhysicsDebugMeshes::PhysicsDebugMeshes()
	{
		Init();
	}

	PhysicsDebugMeshes::~PhysicsDebugMeshes()
	{
		SAT_CORE_ASSERT( !m_BoxMesh, "Box Mesh is still valid! This means that you should of called PhysicsDebugMeshes::Terminate before all the singletons get cleaned up!" );
	}

	void PhysicsDebugMeshes::Terminate()
	{
		m_BoxMesh.Reset();
		m_SphereMesh.Reset();
		m_CapsuleMesh.Reset();
		m_MaterialAsset.Reset();
	}

	void PhysicsDebugMeshes::Init()
	{
		m_MaterialAsset = Ref<MaterialAsset>::Create( nullptr );

		m_BoxMesh = Auxiliary::DefaultMeshes::CreateCube( glm::vec3{ 1.0f } );
		m_SphereMesh = Auxiliary::DefaultMeshes::CreateSphere( 0.5f );
		m_CapsuleMesh = Auxiliary::DefaultMeshes::CreateCapsule( 0.5f, 1.0f );

		m_BoxMesh->GetMaterialRegistry()->AddAsset( m_MaterialAsset );
		m_SphereMesh->GetMaterialRegistry()->AddAsset( m_MaterialAsset );
		m_CapsuleMesh->GetMaterialRegistry()->AddAsset( m_MaterialAsset );
	}

}
