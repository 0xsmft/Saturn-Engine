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

#include "Saturn/NodeEditor/NodeEditorBase.h"
#include "Saturn/NodeEditor/NodeEditorBlueprintNode.h"

namespace Saturn {
	
	SCLASS()
	class MathsAddFloats : public NodeEditorBlueprintNode
	{
		SAT_DECLARE_CLASS( MathsAddFloats, NodeEditorBlueprintNode );
	public:
		MathsAddFloats();
		MathsAddFloats( const std::string& rName );

		virtual ~MathsAddFloats();

	private:
		void CreateNode();
	};

	SCLASS()
	class MathsSubFloats : public NodeEditorBlueprintNode
	{
		SAT_DECLARE_CLASS( MathsSubFloats, NodeEditorBlueprintNode );
	public:
		MathsSubFloats();
		MathsSubFloats( const std::string& rName );

		virtual ~MathsSubFloats();

	private:
		void CreateNode();
	};

	SCLASS()
	class MathsMulFloats : public NodeEditorBlueprintNode
	{
		SAT_DECLARE_CLASS( MathsMulFloats, NodeEditorBlueprintNode );
	public:
		MathsMulFloats();
		MathsMulFloats( const std::string& rName );

		virtual ~MathsMulFloats();

	private:
		void CreateNode();
	};

	SCLASS()
	class MathsDivideFloats : public NodeEditorBlueprintNode
	{
		SAT_DECLARE_CLASS( MathsDivideFloats, NodeEditorBlueprintNode );
	public:
		MathsDivideFloats();
		MathsDivideFloats( const std::string& rName );

		virtual ~MathsDivideFloats();

	private:
		void CreateNode();
	};

	SCLASS()
	class MathsGreaterThanFloats : public NodeEditorBlueprintNode
	{
		SAT_DECLARE_CLASS( MathsGreaterThanFloats, NodeEditorBlueprintNode );
	public:
		MathsGreaterThanFloats();
		MathsGreaterThanFloats( const std::string& rName );

		virtual ~MathsGreaterThanFloats();

		virtual NodeEditorTaskBase* ConvertToTask() override;

	private:
		void CreateNode();
	};

	SCLASS()
	class MathsLessThanFloats : public NodeEditorBlueprintNode
	{
		SAT_DECLARE_CLASS( MathsLessThanFloats, NodeEditorBlueprintNode );
	public:
		MathsLessThanFloats();
		MathsLessThanFloats( const std::string& rName );

		virtual ~MathsLessThanFloats();

		virtual NodeEditorTaskBase* ConvertToTask() override;

	private:
		void CreateNode();
	};

	SCLASS()
	class MathsNot : public NodeEditorBlueprintNode
	{
		SAT_DECLARE_CLASS( MathsNot, NodeEditorBlueprintNode );
	public:
		MathsNot();
		MathsNot( const std::string& rName );

		virtual ~MathsNot();

		virtual NodeEditorTaskBase* ConvertToTask() override;

	private:
		void CreateNode();
	};

	SCLASS()
	class MathsOr : public NodeEditorBlueprintNode
	{
		SAT_DECLARE_CLASS( MathsOr, NodeEditorBlueprintNode );
	public:
		MathsOr();
		MathsOr( const std::string& rName );

		virtual ~MathsOr();

		virtual NodeEditorTaskBase* ConvertToTask() override;

	private:
		void CreateNode();
	};

	//////////////////////////////////////////////////////////////////////////
	
	class MathsNodesAuxiliary
	{
	public:
		static SharedPtr<NodeEditorNodeBase> DrawContextMenu( SharedPtr<NodeEditor> nodeEditor );
	};
}
