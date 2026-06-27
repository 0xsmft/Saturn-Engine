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
#include "PhysicsSurfaceRegistryAsset.h"

namespace Saturn {

	PhysicsSurfaceRegistryAsset::PhysicsSurfaceRegistryAsset()
	{
	}

	PhysicsSurfaceRegistryAsset::PhysicsSurfaceRegistryAsset( const Ref<Asset>& rBase )
		: Asset( rBase )
	{
	}

	PhysicsSurfaceRegistryAsset::~PhysicsSurfaceRegistryAsset()
	{
	}

	void PhysicsSurfaceRegistryAsset::AddSurfaceType( const std::string& rName )
	{
		if( !DoesSurfaceTypeExist( rName ) )
		{
			m_Surfaces.emplace_back( rName );
		}
	}

	bool PhysicsSurfaceRegistryAsset::DoesSurfaceTypeExist( const std::string& rName )
	{
		return std::find_if( m_Surfaces.begin(), m_Surfaces.end(), 
			[ rName ]( const auto& rCandidate )
		{ 
			return rCandidate.Name == rName;
		} ) != m_Surfaces.end();
	}

}
