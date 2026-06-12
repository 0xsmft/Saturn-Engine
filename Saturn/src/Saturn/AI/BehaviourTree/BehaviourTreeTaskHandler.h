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

#include "Saturn/NodeEditor/NodeEditorTaskHandler.h"

namespace Saturn {

	class AIAgentEntity;

	class BehaviourTreeTaskHandler : public NodeEditorTaskHandler
	{
	public:
		BehaviourTreeTaskHandler();
		virtual ~BehaviourTreeTaskHandler();

		virtual void Tick( Timestep ts ) override;

	public:
		void SetAgent( AIAgentEntity* pAgent ) { m_pAIAgentEntity = pAgent; }
		AIAgentEntity* GetTargetAgent() const { return m_pAIAgentEntity; }

	private:
		// The current Agent that we are trying to control
		// #ReplaceRawPtrOrRefWithWeakRef, non owning ptr, should be converted to a weak ptr because we don't want to stop the entity from delete, we are "child" of it
		AIAgentEntity* m_pAIAgentEntity = nullptr;
	};
	
}
