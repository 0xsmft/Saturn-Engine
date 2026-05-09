/********************************************************************************************
*                                                                                           *
*                                                                                           *
*                                                                                           *
* MIT License                                                                               *
*                                                                                           *
* Copyright (c) 2026 BEAST                                                                  *
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

#include "Saturn/Core/IndexedMap.h"

#include <unordered_map>

namespace Saturn {

	struct AnimGraphNodeAndTaskInfo;
	class NodeEditorVariable;

	class NodeTaskCache
	{
	public:
		using NodeTaskCacheMap = std::vector<Ref<NodeEditorTaskBase>>;
		using NodeVariableMap = std::unordered_map<UUID, Ref<NodeEditorVariable>>;

	public:
		NodeTaskCache();
		~NodeTaskCache();

		//
		// Build the master task list for this cache.
		// 
		// @param rOrder - the order in which the tasks will run from.
		//				   index 0 should be the first node that will run.
		//                 This order is volatile meaning that it may change after InstantiateNewTaskList()
		//				   is called the order may change. For example, take AnimGraphs, their order is dependent
		//				   on conditions and the state machine.
		//				   SoundGraphs do have a fixed order however that can be broken if there is a RandomSound node.
		//
		// @param cacheVariables - specify whether to cache variables or not. Default value is true
		//
		void BuildMasterList( const std::vector<SharedPtr<NodeEditorNodeBase>>& rOrder, bool cacheVariables = true );

		//
		// @see: BuildMasterList
		// 
		// Builds the master list for an animation graph.
		//
		void BuildMasterListForAnimGraph( NodeEditor* pEditor, const IndexedMap<UUID, AnimGraphNodeAndTaskInfo>& rOrder, bool cacheVariables = true );

		//
		// Clear the list.
		//
		void Clear();

	public:
		[[nodiscard]] bool IsListEmpty() const { return m_Tasks.empty(); }

		const NodeTaskCacheMap& GetMasterListForSerialisation() const { return m_Tasks; }
		NodeTaskCacheMap& GetMasterListForSerialisation() { return m_Tasks; }

		std::unordered_map<UUID, Ref<NodeEditorVariable>>& GetMasterVariablesListForSerialisation() { return m_EditorVariables; }

		NodeTaskCacheMap InstantiateNewTaskList( NodeEditorTaskHandler* pHandler ) const;

#if !defined(SAT_DIST)
		[[nodiscard]] bool IsDirty() const { return m_IsDirty; }
		void MarkDirty() { m_IsDirty = true; }
		void MarkClean() { m_IsDirty = false; }
#endif

	private:
		void CacheVariables( NodeEditor* pEditor );

	private:
		// Task map, global, any new TaskHandler will use the map and create their own global copy of it.
		NodeTaskCacheMap m_Tasks;

		// Variable map, global, any new TaskHandler will use the map and create their own global copy of it.
		NodeVariableMap m_EditorVariables;

#if !defined(SAT_DIST)
		bool m_IsDirty = false;
#endif
	};
	
}
