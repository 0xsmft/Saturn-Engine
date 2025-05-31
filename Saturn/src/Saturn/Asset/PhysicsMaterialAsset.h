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

#include "Asset.h"

#include "PxPhysicsAPI.h"

namespace Saturn {

	enum class PhysicsMaterialFlags
	{
		None = 0,
		DisableFriction = BIT( 0 ),
		DisableHighFriction = BIT( 1 ),
		ImprovedPatchFriction = BIT( 2 )
	};

	class PhysicsMaterialAsset : public Asset
	{
	public:
		PhysicsMaterialAsset( float StaticFriction, float DynamicFriction, float Restitution, PhysicsMaterialFlags flags = PhysicsMaterialFlags::None );
		PhysicsMaterialAsset( const Ref<Asset>& rBase, float StaticFriction, float DynamicFriction, float Restitution, PhysicsMaterialFlags flags = PhysicsMaterialFlags::None );
		virtual ~PhysicsMaterialAsset();

		void SetStaticFriction( float val );
		void SetDynamicFriction( float val );
		void SetRestitution( float val );
		 
		float GetStaticFriction()  const { return m_StaticFriction; }
		float GetDynamicFriction() const { return m_DynamicFriction; }
		float GetRestitution()     const { return m_Restitution; }

		void SetFlag( PhysicsMaterialFlags flag, bool value );
		[[nodiscard]] bool IsFlagSet( PhysicsMaterialFlags flag ) const { return ( m_Flags & (uint32_t)flag ) != 0; }
		uint32_t GetFlags() const { return m_Flags; }

		physx::PxMaterial& GetMaterial() { return *m_Material; }
		const physx::PxMaterial& GetMaterial() const { return *m_Material; }

	private:
		float m_StaticFriction = 0.6f;
		float m_DynamicFriction = 0.6f;
		float m_Restitution = 0.0f;

		physx::PxMaterial* m_Material = nullptr;
		uint32_t m_Flags = -1;

	private:
		friend class PhysicsMaterialAssetViewer;
	};

}