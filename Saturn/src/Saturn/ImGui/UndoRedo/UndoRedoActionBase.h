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

#include "Saturn/Core/UUID.h"
#include "Saturn/Core/Ref.h"

#include <string>

namespace Saturn {

	// The base class for all undo redo actions
	// Most UndoRedo action are added to the global list AFTER it has been executed
	class UndoRedoActionBase : public RefTarget
	{
	public:
		UndoRedoActionBase( const std::string& rName ) : m_Name( rName ) {}
		virtual ~UndoRedoActionBase() = default;

		virtual void Undo() = 0;
		virtual void Redo() = 0;

	public:
		const std::string& GetName() const { return m_Name; }
		UUID GetIdentifier() const { return m_Identifier; }

		void SetIdentifier( UUID identifier ) { m_Identifier = identifier; }

	private:
		std::string m_Name;

		// Every action should have an identifier that states who it's associated with. For example any entity action should have the entity handle as the identifier, so when the entity is deleted the action can be removed.
		UUID m_Identifier{};
	};

	template<typename Ty>
	class UndoRedoActionModifyT : public UndoRedoActionBase
	{
	public:
		UndoRedoActionModifyT( const std::string& rName, Ty* pTarget, const Ty& rOldValue, const Ty& rNewValue )
			: UndoRedoActionBase( rName ), m_OriginalValue( rOldValue ), m_CurrentValue( rNewValue ), m_pTarget( pTarget ) {}
		
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

	using UndoRedoActionModifyInt8 = UndoRedoActionModifyT<int8_t>;
	using UndoRedoActionModifyInt16 = UndoRedoActionModifyT<int16_t>;
	using UndoRedoActionModifyInt32 = UndoRedoActionModifyInt;
	using UndoRedoActionModifyInt64 = UndoRedoActionModifyT<int64_t>;
	using UndoRedoActionModifyUInt8 = UndoRedoActionModifyT<uint8_t>;
	using UndoRedoActionModifyUInt16 = UndoRedoActionModifyT<uint16_t>;
	using UndoRedoActionModifyUInt32 = UndoRedoActionModifyT<uint32_t>;
	using UndoRedoActionModifyUInt64 = UndoRedoActionModifyT<uint64_t>;
	using UndoRedoActionModifySizeT = UndoRedoActionModifyT<size_t>;

	// Char types
	using UndoRedoActionModifyChar = UndoRedoActionModifyT<char>;
	using UndoRedoActionModifyWChar = UndoRedoActionModifyT<wchar_t>;

	// Maths
	using UndoRedoActionModifyVec2 = UndoRedoActionModifyT<glm::vec2>;
	using UndoRedoActionModifyVec3 = UndoRedoActionModifyT<glm::vec3>;
	using UndoRedoActionModifyVec4 = UndoRedoActionModifyT<glm::vec4>;
	using UndoRedoActionModifyMat4 = UndoRedoActionModifyT<glm::mat4>;
	
	// Strings
	using UndoRedoActionModifyString = UndoRedoActionModifyT<std::string>;
	using UndoRedoActionModifyWString = UndoRedoActionModifyT<std::wstring>;

}
