/********************************************************************************************
*                                                                                           *
*                                                                                           *
*                                                                                           *
* MIT License                                                                               *
*                                                                                           *
* Copyright (c) 2020 - 2023 BEAST                                                           *
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
#include "PhysicsMaterialAsset.h"

#include "PhysicsAuxiliary.h"
#include "PhysicsFoundation.h"

namespace Saturn {

	PhysicsMaterialAsset::PhysicsMaterialAsset( float Friction, float Restitution )
		: Asset(), m_JoltMaterial( new PhysicsInternalMaterial( Friction, Restitution ) )
	{
	}

	PhysicsMaterialAsset::PhysicsMaterialAsset( const Ref<Asset>& rBase, float Friction, float Restitution )
		: Asset( rBase ), m_JoltMaterial( new PhysicsInternalMaterial( Friction, Restitution ) )
	{
	}

	PhysicsMaterialAsset::~PhysicsMaterialAsset()
	{	
	}

	void PhysicsMaterialAsset::SetFriction( float val )
	{
		m_JoltMaterial->SetFriction( val );
	}

	void PhysicsMaterialAsset::SetRestitution( float val )
	{
		m_JoltMaterial->SetRestitution( val );
	}

	void PhysicsMaterialAsset::SetSurfaceName( const std::string& rName )
	{
		m_SurfaceName = rName;
	}

}
