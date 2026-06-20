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

namespace Saturn {

	//////////////////////////////////////////////////////////////////////////
	// MODIFY TRANSFORMATION

	class UndoRedoActionModifyEntityTransformation : public UndoRedoActionBase
	{
	public:
		UndoRedoActionModifyEntityTransformation() = default;

		UndoRedoActionModifyEntityTransformation( SharedPtr<Entity> entity, const glm::mat4& rOriginalRotation, const glm::mat4& rCurrentValue )
			: UndoRedoActionBase( "Modify Entity Transformation" ), m_OriginalTransform( rOriginalRotation ), m_CurrentTransform( rCurrentValue )
		{
			m_pTransformComponent = &entity->GetComponent<TransformComponent>();
		}

		~UndoRedoActionModifyEntityTransformation()
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
