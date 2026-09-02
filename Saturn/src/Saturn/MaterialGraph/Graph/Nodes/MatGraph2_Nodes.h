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

#include "Saturn/NodeEditor/NodeEditorBlueprintNode.h"

namespace Saturn {
	
	SCLASS()
	class SMatGraph2_OutputNode : public NodeEditorBlueprintNode
	{
		SAT_DECLARE_CLASS( SMatGraph2_OutputNode, NodeEditorBlueprintNode );
	public:
		SMatGraph2_OutputNode();
		virtual ~SMatGraph2_OutputNode();

		virtual NodeEditorTaskBase* ConvertToTask() override;

	private:
		void CreateNode();
	};

	SCLASS()
	class SMatGraph2_TextureSampleNode : public NodeEditorBlueprintNode
	{
		SAT_DECLARE_CLASS( SMatGraph2_TextureSampleNode, NodeEditorBlueprintNode );
	public:
		SMatGraph2_TextureSampleNode();
		virtual ~SMatGraph2_TextureSampleNode();

		virtual NodeEditorTaskBase* ConvertToTask() override;

	private:
		void CreateNode();
	};

	SCLASS()
	class SMatGraph2_ConstantColourNode : public NodeEditorBlueprintNode
	{
		SAT_DECLARE_CLASS( SMatGraph2_ConstantColourNode, NodeEditorBlueprintNode );
	public:
		SMatGraph2_ConstantColourNode();
		virtual ~SMatGraph2_ConstantColourNode();

		virtual NodeEditorTaskBase* ConvertToTask() override;

	public:
		void SetColour( const glm::vec3& rColor );

	private:
		void CreateNode();
	};
}
