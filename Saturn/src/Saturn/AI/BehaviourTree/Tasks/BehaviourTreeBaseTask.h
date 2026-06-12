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

#include "Saturn/AI/BehaviourTree/Blackboard.h"

#include "Saturn/NodeEditor/NodeEditorTaskBase.h"

namespace Saturn {

	class BehaviourTreeNodeBase;
	class BehaviourTreeNodeEditor;

	SCLASS()
	class BehaviourTreeBaseTask : public NodeEditorTaskBase
	{
		SAT_DECLARE_CLASS_MOVE( BehaviourTreeBaseTask, NodeEditorTaskBase )
	public:
		BehaviourTreeBaseTask() = default;
		virtual ~BehaviourTreeBaseTask() = default;

		void SetBlackboard( Blackboard* pBlackboard );

#if !defined(SAT_DIST)
	public:
		[[nodiscard]] virtual const char* GetTaskName() const { return "Base Task"; }

		// allows us to filter if the node that we represent is spawnable or not.
		[[nodiscard]] virtual bool IsSpawnableNode() const { return false; }

		virtual void OnRenderExtra() {}
		virtual void RenderDetails() {}
#endif

	protected:
		template<typename CppType>
		std::optional<CppType> TryRetrieveBBKey( Saturn::UUID variableID )
		{
			if( !m_pRTBlackboard ) return std::nullopt;

			return m_pRTBlackboard->GetKeyValue<CppType>( variableID );
		}

	protected:
		// Non owning ptr, owned by BehaviourTreeNodeEditor
		// Runtime blackboard information
		Blackboard* m_pRTBlackboard = nullptr;
		Saturn::UUID m_RTBlackboardVariableID = 0;
	};
}
