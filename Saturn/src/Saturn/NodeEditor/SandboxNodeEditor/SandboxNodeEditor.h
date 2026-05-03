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

#include "SandboxNodeEditorTaskHandler.h"

#include "Saturn/NodeEditor/UI/NodeEditor.h"

namespace Saturn {

	//
	// SandboxNodeEditor 
	// 
	// SandboxNodeEditor is a debugging, development-only and sandbox environment NodeEditor.
	// It's sole purpose is to help us implement new features in NodeEditors, without breaking 
	// existing NodeEditors.
	//
	class SandboxNodeEditor : public NodeEditor
	{
	public:
		SandboxNodeEditor();
		virtual ~SandboxNodeEditor();

		// NB: Typically, you'd do this in a Destroy function or the destructor
		//     for this NodeEditor, we need to do it in the function because this NodeEditor
		//     is kept alive by the Editor.
		void BuildTaskCache();

		virtual void OnUpdate( Timestep ts ) override;

#if !defined(SAT_DIST)
	public:
		virtual void OnImGuiRender() override;
		virtual void OnTopBarRender() override;
#endif

	private:
		void DrawRuntimeControl();
		void ClearEditor();

	private:
		// NB: Typically, you'd store this elsewhere and anyway from the NodeEditor because the NodeEditor
		//	   will not exist on Dist and you'd use tasks when in Runtime or Dist.
		//     but for example purposes it's fine here inside of the NodeEditor.
		Ref<SandboxNodeEditorTaskHandler> m_TaskHandler;

		bool m_ShowRuntimeControl = false;
	};
	
}
