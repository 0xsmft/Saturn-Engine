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

#include "NodeEditorTaskBase.h"
#include "NodeEditorVariableLocator.h"
#include "NodeEditorVariable.h"

#include <map>

namespace Saturn {

	class NodeTaskCache;

	class NodeEditorTaskHandler : public RefTarget
	{
	public:
		NodeEditorTaskHandler() = default;
		virtual ~NodeEditorTaskHandler();
		
	public:
		virtual void Tick( Timestep ts );

	public:
		void Init( const NodeTaskCache& rCache );

	public:
		template<typename Ty>
		void RegisterLocator( UUID nodeID, size_t pinIndex, Ty* pAddress ) 
		{
			SAT_CORE_ASSERT( nodeID != 0 );

			auto& rLocators = m_Locators[ nodeID ];
			if( pinIndex >= rLocators.size() )
			{
				rLocators.resize( pinIndex + 1 );
			}

			rLocators[ pinIndex ].Set( pAddress );
		}

		//
		// Register a new locator but have the Task handler own and store it.
		// 
		// So, it must allocate Ty on the heap!
		//
		template<typename Ty>
		Ty* RegisterLocatorStorage( UUID nodeID, size_t pinIndex )
		{
			auto& rLocators = m_Locators[ nodeID ];
			if( pinIndex >= rLocators.size() )
			{
				rLocators.resize( pinIndex + 1 );
			}

			rLocators[ pinIndex ].Set( new Ty(), true );

			return ( Ty* )rLocators[ pinIndex ].Get();
		}

		template<typename Ty>
		Ty* AccessLocator( UUID id, size_t pinIndex ) const
		{
			const auto itr = m_Locators.find( id );
			if( itr == m_Locators.end() )
				return nullptr;

			if( pinIndex >= itr->second.size() )
				return nullptr;

			return ( Ty* ) itr->second[ pinIndex ].Get();
		}

		Ref<NodeEditorVariable> GetVariable( UUID id );
		Ref<NodeEditorVariable> GetVariable( const std::string& rName );

	protected:
		void ResetAllTasks();
	
	protected:
		// All tasks in the tree.
		std::vector<Ref<NodeEditorTaskBase>> m_Tasks;
		Ref<NodeEditorTaskBase> m_CurrentTask;
		size_t m_CurrentTaskIndex = 0;

		// Variables from the NodeEditor copied into the task handler.
		//		           VAR ID -> VARIABLE 
		std::unordered_map<UUID, Ref<NodeEditorVariable>> m_EditorVariables;

		// Locators
		//		NODE ID -> LOCATORS (PER OUTPUT PIN)
		std::map<UUID, std::vector<NodeEditorVariableLocator>> m_Locators;
	
	private:
		friend class NodeTaskCache;
	};
	
}
