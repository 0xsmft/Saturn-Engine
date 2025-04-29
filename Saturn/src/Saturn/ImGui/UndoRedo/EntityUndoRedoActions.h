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

#include "UndoRedoActionBase.h"

namespace Saturn {

	enum class UndoRedoActionEntityComponentOp
	{
		AddComponent,
		RemoveComponent,
	};

	template<UndoRedoActionEntityComponentOp ComponentActionType, typename C>
	class UndoRedoActionEntityComponent : public UndoRedoActionBase
	{
	public:
		UndoRedoActionEntityComponent( const std::string& rName, Ref<Entity> entity ) : UndoRedoActionBase( rName  ), m_EntityHandle( entity->GetHandle() ), m_pScene( entity->GetScene() ) {}	

		UndoRedoActionEntityComponent( Ref<Entity> entity )
			: UndoRedoActionBase( ComponentActionType == UndoRedoActionEntityComponentOp::AddComponent ? "Add Component" : "Remove Component" ), m_EntityHandle( entity->GetHandle() ), m_pScene( entity->GetScene() )
		{
		}

	public:
		void Undo() override
		{
			if constexpr( ComponentActionType == UndoRedoActionEntityComponentOp::AddComponent ) 
			{
				// Remove
				if( m_pScene )
					m_pScene->RemoveComponent<C>( m_EntityHandle );
			}
			else if constexpr( ComponentActionType == UndoRedoActionEntityComponentOp::RemoveComponent )
			{
				// Add
				if( m_pScene )
					m_pScene->AddComponent<C>( m_EntityHandle );
			}
		}

		void Redo() override
		{
			if constexpr( ComponentActionType == UndoRedoActionEntityComponentOp::AddComponent )
			{
				// Add
				if( m_pScene )
					m_pScene->AddComponent<C>( m_EntityHandle );
			}
			else if constexpr( ComponentActionType == UndoRedoActionEntityComponentOp::RemoveComponent )
			{
				// Remove
				if( m_pScene )
					m_pScene->RemoveComponent<C>( m_EntityHandle );
			}
		}

	private:
		entt::entity m_EntityHandle;
		Scene* m_pScene = nullptr;
	};

	template<typename Component>
	using UndoRedoActionAddComponent = UndoRedoActionEntityComponent<UndoRedoActionEntityComponentOp::AddComponent, Component>;

	template<typename Component>
	using UndoRedoActionRemoveComponent = UndoRedoActionEntityComponent<UndoRedoActionEntityComponentOp::RemoveComponent, Component>;

	//////////////////////////////////////////////////////////////////////////
	// MODIFY TRANSFORMATION

	class UndoRedoActionModifyTransformation : public UndoRedoActionBase
	{
	public:
		UndoRedoActionModifyTransformation() = default;

		UndoRedoActionModifyTransformation( Ref<Entity> entity, const glm::mat4& rOriginalRotation, const glm::mat4& rCurrentValue )
			: UndoRedoActionBase( "Modify Entity Transformation" ), m_OriginalTransform( rOriginalRotation ), m_CurrentTransform( rCurrentValue )
		{
			m_pTransformComponent = &entity->GetComponent<TransformComponent>();
		}

		~UndoRedoActionModifyTransformation()
		{
			m_pTransformComponent = nullptr;
		}

	public:
		void Undo() override
		{
			if( m_pTransformComponent )
			{
				m_pTransformComponent->SetTransform( m_OriginalTransform );
			}
		}

		void Redo() override
		{
			if( m_pTransformComponent )
			{
				m_pTransformComponent->SetTransform( m_CurrentTransform );
			}
		}

	private:
		TransformComponent* m_pTransformComponent = nullptr;

		glm::mat4 m_OriginalTransform{};
		glm::mat4 m_CurrentTransform{};
	};
}