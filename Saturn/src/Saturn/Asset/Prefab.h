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
#include "Saturn/Scene/Entity.h"

#include <entt.hpp>

namespace Saturn {

	class Scene;

	//////////////////////////////////////////////////////////////////////////
	// Prefab also known as ClassInstance
	//
	// This is not like a normal prefab. Think of this like an instance of another class (class means Entity or type defined in the game).
	// Prefabs can only represent one root entity however, that entity can have children
	// Prefabs can not depend on other prefabs, this means that any child must not have a prefab component, only the root entity can (which would point to this prefab instance)
	class Prefab : public Asset
	{
	public:
		Prefab();
		Prefab( const Ref<Asset>& rBase );
		virtual ~Prefab();

		// Convert an entity into a prefab
		void InitPrefab( const SharedPtr<Entity>& srcEntity );
		
		SharedPtr<Entity> PrefabToEntity( Ref<Scene> SceneToSpawnIn );

		Ref<Scene> GetScene() { return m_Scene; }
		const Ref<Scene> GetScene() const { return m_Scene; }

	public:
		//////////////////////////////////////////////////////////////////////////
		// #WARNING This should not be confused with AssetSerialisers. This is for raw binary serialisation!
		// ASSET BUNDLE ONLY!
		void SerialisePrefab( std::ofstream& rStream );
		void DeserialisePrefab( std::istream& rStream );

	private:
		SharedPtr<Entity> CreateFromEntity( SharedPtr<Entity> srcEntity );
		SharedPtr<Entity> CreateChildren( const SharedPtr<Entity>& parent, Ref<Scene> Scene );

		void ConvertSceneEntityIntoPrefabEntity( const SharedPtr<Entity> srcEntity );

	private:
		SharedPtr<Entity> m_Entity;

		Ref<Scene> m_Scene;

//		UUID m_ModificationID;

	private:
		friend class PrefabSerialiser;
	};

}
