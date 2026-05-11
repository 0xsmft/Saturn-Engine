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

#include "Saturn/NodeEditor/NodeEditorTaskBase.h"

namespace Saturn {

	SCLASS();
	class SMaterialGraphColorPickerTask : public NodeEditorTaskBase
	{
		SAT_DECLARE_CLASS( SMaterialGraphColorPickerTask, NodeEditorTaskBase );
	public:
		SMaterialGraphColorPickerTask();
		virtual ~SMaterialGraphColorPickerTask();

	public:
#if !defined(SAT_DIST)
		virtual void PreInitialiseTask( NodeEditor* pEditor, NodeEditorNodeBase* pNode ) override;
#endif
		virtual void InitialiseTaskWithOther( NodeEditorTaskHandler* pHandler, NodeEditorTaskBase* pOther ) override;

		virtual NodeEditorTaskState Tick( Timestep ts ) override;
		virtual void Reset() override;
		virtual void Serialise( std::ofstream& rStream ) const override;
		virtual void Deserialise( FDependentIStream& rStream ) override;

	private:
		glm::vec3 m_Color{ 1.0f };
	};
	
	SCLASS();
	class SMaterialGraphOutputNodeTask : public NodeEditorTaskBase
	{
		SAT_DECLARE_CLASS( SMaterialGraphOutputNodeTask, NodeEditorTaskBase );
	public:
		SMaterialGraphOutputNodeTask();
		virtual ~SMaterialGraphOutputNodeTask();

	public:
#if !defined(SAT_DIST)
		virtual void PreInitialiseTask( NodeEditor* pEditor, NodeEditorNodeBase* pNode ) override;
#endif
		virtual void InitialiseTaskWithOther( NodeEditorTaskHandler* pHandler, NodeEditorTaskBase* pOther ) override;

		virtual NodeEditorTaskState Tick( Timestep ts ) override;
		virtual void Reset() override;
		virtual void Serialise( std::ofstream& rStream ) const override;
		virtual void Deserialise( FDependentIStream& rStream ) override;

	private:
		bool m_AlbedoIsColor = false;
		UUID m_AlbedoID = 0, m_NormalID = 0, m_RoughnessID = 0, m_MetallicID = 0, m_EmissionID = 0;
	};

}
