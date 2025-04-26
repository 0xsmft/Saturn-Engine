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

#include "Saturn/Core/Base.h"
#include "UndoRedoActionBase.h"

#include <stack>

namespace Saturn {

	// In development configurations this class will be owned by the EditorLayer
	class GlobalUndoRedoGroup : public RefTarget
	{
	public:
		static inline GlobalUndoRedoGroup& Get() { return *SingletonStorage::GetSingleton<GlobalUndoRedoGroup>(); }

	public:
		GlobalUndoRedoGroup();
		~GlobalUndoRedoGroup();

	public:
		Ref<UndoRedoActionBase> GlobalUndoRecent();
		Ref<UndoRedoActionBase> GlobalRedoRecent();
		
		void GlobalUndoTo( size_t amount = 0 );
		void GlobalRedoTo( size_t amount = 0 );

		void RemoveIfActionHasIdentifier( UUID identifier );
		
	public:
		void AddAction( Ref<UndoRedoActionBase> action, UUID identifier );
		void RemoveAction( Ref<UndoRedoActionBase> action );

		Ref<UndoRedoActionBase> GetTopUndoAction() { return m_UndoActions.empty() ? nullptr : m_UndoActions.back(); }
		Ref<UndoRedoActionBase> GetTopRedoAction() { return m_RedoActions.empty() ? nullptr : m_RedoActions.back(); }

#if !defined(SAT_DIST)
		void OnImGuiRender( bool* pOpen );
#endif

	private:
		std::vector<Ref<UndoRedoActionBase>> m_RedoActions;
		std::vector<Ref<UndoRedoActionBase>> m_UndoActions;
	};
	
}