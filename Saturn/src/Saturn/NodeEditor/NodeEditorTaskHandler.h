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
#include "DataLine.h"

#include <map>

namespace Saturn {

	class Animator;

	class NodeEditorTaskHandler : public RefTarget
	{
	public:
		NodeEditorTaskHandler() = default;
		virtual ~NodeEditorTaskHandler();

		void Init( SharedPtr<NodeEditorBase> nodeEditor );
		virtual void Tick( Timestep ts );

		void InsertDataLine( UUID linkID, const DataLine& rLine );
		DataLine* GetDataLine( UUID linkID );
		bool DoesDataLineExist( UUID linkID );

	protected:
		void ResetAllTasks();
	
	protected:
		// All tasks in the tree
		//       NODE ID -> TASK*
		std::vector<Ref<NodeEditorTaskBase>> m_Tasks;
		Ref<NodeEditorTaskBase> m_CurrentTask;
		size_t m_CurrentTaskIndex = 0;

		std::map<UUID, DataLine> m_Lines;
	};
	
}
