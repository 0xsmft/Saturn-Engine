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
#include "Prefab2.h"

#include "Saturn/GameFramework/SClass.h"

namespace Saturn {

	Prefab2::Prefab2()
	{
	}

	Prefab2::Prefab2( const Ref<Asset>& rBase )
	{
	}

	Prefab2::~Prefab2()
	{
	}

	void Prefab2::CreatePrefabFromSceneEntity( SharedPtr<Entity> entity )
	{
		PrefabEntity prefabEntity;
		prefabEntity.ID = entity->GetUUID();
		prefabEntity.ParentID = entity->GetParent();
		prefabEntity.ClassHash = entity->GetClass()->GetHash();

		m_Entities[ ( uint32_t ) entity->GetHandle() ] = prefabEntity;

		for( const auto& rChildId : entity->GetChildren() )
		{
			SharedPtr<Entity> child = entity->GetScene()->FindEntityByID( rChildId );
			if( child )
			{
				CreatePrefabFromSceneEntity( child );
			}
		}
	}

}
