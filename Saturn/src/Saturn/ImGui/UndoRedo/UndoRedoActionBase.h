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

#include "Saturn/Core/Ref.h"
#include <string>

namespace Saturn {

	class UndoRedoActionBase : public RefTarget
	{
	public:
		UndoRedoActionBase( const std::string& rName, const std::string& rDescription ) : m_Name( rName ), m_Description( rDescription ) {}
		virtual ~UndoRedoActionBase() = default;

		virtual void Undo() = 0;
		virtual void Redo() = 0;

	public:
		const std::string& GetName() const { return m_Name; }
		const std::string& GetDescription() const { return m_Description; }

	private:
		std::string m_Name;
		std::string m_Description;

		uint64_t m_ActionType = 0;
	};

	template<typename Ty>
	class UndoRedoActionModifyT : public UndoRedoActionBase
	{
	public:
		UndoRedoActionModifyT( const std::string& rName, const std::string& rDescription, Ty* pTarget, const Ty& rOldValue, const Ty& rNewValue )
			: UndoRedoActionBase( rName, rDescription ), m_OriginalValue( rOldValue ), m_CurrentValue( rNewValue ), m_pTarget( pTarget ) {}
		
		void Undo() override
		{
			if( m_pTarget )
				*m_pTarget = m_OriginalValue;
		}
		
		void Redo() override
		{
			if( m_pTarget )
				*m_pTarget = m_CurrentValue;
		}
	
	private:
		Ty m_OriginalValue;
		Ty m_CurrentValue;

		Ty* m_pTarget = nullptr;
	};

	using UndoRedoActionModifyInt = UndoRedoActionModifyT<int>;
	using UndoRedoActionModifyFloat = UndoRedoActionModifyT<float>;
	using UndoRedoActionModifyDouble = UndoRedoActionModifyT<double>;
	using UndoRedoActionModifyBool = UndoRedoActionModifyT<bool>;

	// Maths
	using UndoRedoActionModifyVec2 = UndoRedoActionModifyT<glm::vec2>;
	using UndoRedoActionModifyVec3 = UndoRedoActionModifyT<glm::vec3>;
	using UndoRedoActionModifyVec4 = UndoRedoActionModifyT<glm::vec4>;
	using UndoRedoActionModifyMat4 = UndoRedoActionModifyT<glm::mat4>;
	
}