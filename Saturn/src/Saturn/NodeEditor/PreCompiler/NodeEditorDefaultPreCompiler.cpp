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
#include "NodeEditorDefaultPreCompiler.h"

namespace Saturn {

	std::vector<NodeEditorPreCompileError> NodeEditorDefaultPreCompiler::PreCompile()
	{
		std::vector<NodeEditorPreCompileError> errors;

		if( !m_NodeEditor ) 
		{
			errors.emplace_back( NodeEdPreCompCategory_Standard, NodeEdPreCompError_InternalError );
			return errors;
		}

		// Walk through all the node and find any pins with the PinFlag_RequiredForEvaulation.
		for( const auto& rNode : m_Order )
		{
			for( const auto& rInput : rNode->Inputs )
			{
				if( rInput->IsFlagSet( PinFlag_RequiredForEvaluation ) && !m_NodeEditor->IsLinked( rInput->ID ) )
				{
					errors.emplace_back( NodeEdPreCompCategory_Standard, NodeEdPreCompError_MissingRequiredLink );
				}
			}
		}

		return errors;
	}

}
