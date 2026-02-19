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

#include "sppch.h"
#include "MathsNodeLibrary.h"

#include "MathsNodes.h"

#include "Saturn/GameFramework/Core/ClassMetadataHandler.h"

namespace Saturn {

	SharedPtr<MathsAddFloats> MathsNodeLibrary::SpawnMathAdd( SharedPtr<NodeEditorBase> nodeEditor )
	{
		SharedPtr<MathsAddFloats> node = NewObject<MathsAddFloats>( nodeEditor.Get() );
		nodeEditor->AddNode( node );

		return node;
	}

	SharedPtr<MathsSubFloats> MathsNodeLibrary::SpawnMathSub( SharedPtr<NodeEditorBase> nodeEditor )
	{
		SharedPtr<MathsSubFloats> node = NewObject<MathsSubFloats>( nodeEditor.Get() );
		nodeEditor->AddNode( node );

		return node;
	}

	SharedPtr<MathsMulFloats> MathsNodeLibrary::SpawnMathMul( SharedPtr<NodeEditorBase> nodeEditor )
	{
		SharedPtr<MathsMulFloats> node = NewObject<MathsMulFloats>( nodeEditor.Get() );
		nodeEditor->AddNode( node );

		return node;
	}

	SharedPtr<MathsDivideFloats> MathsNodeLibrary::SpawnMathDiv( SharedPtr<NodeEditorBase> nodeEditor )
	{
		SharedPtr<MathsDivideFloats> node = NewObject<MathsDivideFloats>( nodeEditor.Get() );
		nodeEditor->AddNode( node );

		return node;
	}

	SharedPtr<MathsLessThanFloats> MathsNodeLibrary::SpawnMathLT( SharedPtr<NodeEditorBase> nodeEditor )
	{
		SharedPtr<MathsLessThanFloats> node = NewObject<MathsLessThanFloats>( nodeEditor.Get() );
		nodeEditor->AddNode( node );

		return node;
	}

	SharedPtr<MathsGreaterThanFloats> MathsNodeLibrary::SpawnMathGT( SharedPtr<NodeEditorBase> nodeEditor )
	{
		SharedPtr<MathsGreaterThanFloats> node = NewObject<MathsGreaterThanFloats>( nodeEditor.Get() );
		nodeEditor->AddNode( node );

		return node;
	}

	SharedPtr<MathsNot> MathsNodeLibrary::SpawnNotBool( SharedPtr<NodeEditorBase> nodeEditor )
	{
		SharedPtr<MathsNot> node = NewObject<MathsNot>( nodeEditor.Get() );
		nodeEditor->AddNode( node );

		return node;
	}

	SharedPtr<MathsOr> MathsNodeLibrary::SpawnOrBool( SharedPtr<NodeEditorBase> nodeEditor )
	{
		SharedPtr<MathsOr> node = NewObject<MathsOr>( nodeEditor.Get() );
		nodeEditor->AddNode( node );

		return node;
	}

}
