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
#include "Entity.h"

#include "Saturn/Serialisation/RawEntitySerialisation.h"

namespace Saturn {

	Entity::Entity()
	{
		m_Scene = GActiveScene;
		m_EntityHandle = m_Scene->CreateHandle();
		
		AddComponent<IdComponent>();
		AddComponent<RelationshipComponent>();
		AddComponent<TransformComponent>();
		AddComponent<TagComponent>().Tag = "Unnamed Entity";
	}

	Entity::Entity( const std::string& rName, UUID Id )
	{
		m_Scene = GActiveScene;
		m_EntityHandle = m_Scene->CreateHandle();

		AddComponent<IdComponent>().ID = Id;
		AddComponent<RelationshipComponent>();
		AddComponent<TransformComponent>();
		AddComponent<TagComponent>().Tag = rName;
	}

	Entity::Entity( const Entity& other )
	{
		this->m_Scene = other.m_Scene;
		this->m_EntityHandle = other.m_EntityHandle;
	}

	Entity::Entity( Scene* scene )
	{
		m_Scene = scene;
		m_EntityHandle = m_Scene->CreateHandle();

		AddComponent<IdComponent>();
		AddComponent<RelationshipComponent>();
		AddComponent<TransformComponent>();
		AddComponent<TagComponent>().Tag = "Unnamed Entity";
	}

	Entity::~Entity()
	{
		m_Scene->RemoveHandle( m_EntityHandle );
		m_EntityHandle = entt::null;

		m_Scene = nullptr;
	}

	void Entity::SetName( const std::string& rName )
	{
		GetComponent<TagComponent>().Tag = rName;
	}

	void Entity::Serialise( const Ref<Entity>& rObject, std::ofstream& rStream )
	{
		RawEntitySerialisation serialiser;
		serialiser.SerialiseEntity( rObject, rStream );
	}

	void Entity::Deserialise( Ref<Entity>& rObject, std::istream& rStream )
	{
		RawEntitySerialisation serialiser;
		serialiser.DeserialiseEntity( rObject, rStream );
	}
}

#include "Saturn/GameFramework/Core/EngineGenerated.h"

SAT_X31_CREATE_AUTO_REG_SPWN( Entity );
