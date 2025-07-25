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
#include "GlobalUndoRedoGroup.h"

#if !defined(SAT_DIST)
#include <imgui.h>
#endif

namespace Saturn {

	GlobalUndoRedoGroup::GlobalUndoRedoGroup()
	{
		SingletonStorage::AddSingleton( this );
	}

	GlobalUndoRedoGroup::~GlobalUndoRedoGroup()
	{
		SingletonStorage::RemoveSingleton( this );
	}

	Ref<UndoRedoActionBase> GlobalUndoRedoGroup::GlobalUndoRecent()
	{
		if( !m_UndoActions.empty() )
		{
			auto& rAction = m_UndoActions.back();

			rAction->Undo();

			m_RedoActions.push_back( rAction );
			m_UndoActions.pop_back();
		
			return m_RedoActions.back();
		}

		return nullptr;
	}

	Ref<UndoRedoActionBase> GlobalUndoRedoGroup::GlobalRedoRecent()
	{
		if( !m_RedoActions.empty() )
		{
			auto& rAction = m_RedoActions.back();

			rAction->Redo();

			m_UndoActions.push_back( rAction );
			m_RedoActions.pop_back();
			
			return m_UndoActions.back();
		}

		return nullptr;
	}

	void GlobalUndoRedoGroup::GlobalUndoTo( size_t amount /*= 0 */ )
	{
		for( size_t i = 0; i < amount; i++ )
		{
			GlobalUndoRecent();
		}
	}

	void GlobalUndoRedoGroup::GlobalRedoTo( size_t amount /*= 0 */ )
	{
		for( size_t i = 0; i < amount; i++ )
		{
			GlobalRedoRecent();
		}
	}

	void GlobalUndoRedoGroup::RemoveIfActionHasIdentifier( UUID identifier )
	{
		auto itr = std::remove_if( m_UndoActions.begin(), m_UndoActions.end(),
			[ identifier ]( const Ref<UndoRedoActionBase>& action )
		{
			return action->GetIdentifier() == identifier;
		} );

		m_UndoActions.erase( itr, m_UndoActions.end() );

		itr = std::remove_if( m_RedoActions.begin(), m_RedoActions.end(),
			[ identifier ]( const Ref<UndoRedoActionBase>& action )
		{
			return action->GetIdentifier() == identifier;
		} );

		m_RedoActions.erase( itr, m_RedoActions.end() );
	}

	void GlobalUndoRedoGroup::AddAction( Ref<UndoRedoActionBase> action, UUID identifier )
	{
		action->SetIdentifier( identifier );
		m_UndoActions.push_back( action );

		// TODO: Also remove the action from the redo stack if it exists there?
		// Remove last action if we exceed the max undo/redo actions
		if( m_UndoActions.size() > MAX_UNDO_REDO_ACTIONS )
		{
			m_UndoActions.erase( std::remove( m_UndoActions.begin(), m_UndoActions.end(), m_UndoActions.front() ) );
		}
	}

#if !defined(SAT_DIST)
	void GlobalUndoRedoGroup::OnImGuiRender( bool* pOpen )
	{
		if( ImGui::Begin( "Global Undo Redo Group", pOpen ) )
		{
			ImGui::Text( "Top of undo stack:" );
			ImGui::Text( "%s", m_UndoActions.empty() ? "<empty>" : m_UndoActions.back()->GetName().c_str() );
			ImGui::Text( "Size of undo stack:" );
			ImGui::Text( "%i", m_UndoActions.size() );

			ImGui::Text( "Top of redo stack:" );
			ImGui::Text( "%s", m_RedoActions.empty() ? "<empty>" : m_RedoActions.back()->GetName().c_str() );
			ImGui::Text( "Size of redo stack:" );
			ImGui::Text( "%i", m_RedoActions.size() );

			// Undo actions
			ImGui::Text( "Undo Actions:" );
			for( size_t i = 0; i < m_UndoActions.size(); i++ )
			{
				ImGui::Text( "%i: %s", i, m_UndoActions[ i ]->GetName().c_str() );
			}

			// Redo actions
			ImGui::Text( "Redo Actions:" );
			for( size_t i = 0; i < m_RedoActions.size(); i++ )
			{
				ImGui::Text( "%i: %s", i, m_RedoActions[ i ]->GetName().c_str() );
			}

			ImGui::BeginHorizontal( "##ACTIONRC" );

			if( ImGui::Button( "Undo" ) )
			{
				GlobalUndoRecent();
			}

			ImGui::Spring();

			if( ImGui::Button( "Redo" ) )
			{
				GlobalRedoRecent();
			}

			ImGui::Spring();
			ImGui::EndHorizontal();
			
			ImGui::End();
		}
	}
#endif

}
