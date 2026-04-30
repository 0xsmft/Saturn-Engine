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

	NodeEditorPreCompileResult NodeEditorDefaultPreCompiler::PreCompile()
	{
		NodeEditorPreCompileResult result;

		if( !m_NodeEditor ) 
		{
			result.Messages.emplace_back( 0llu, 0llu, NodeEdPreCompCategory_Standard, NodeEdPreCompError_InternalError );
			result.Succeeded = false;

			return result;
		}

		// Walk through all the node and find any pins with the PinFlag_RequiredForEvaulation.
		for( const auto& rNode : m_Order )
		{
			bool anyPinsLinked = false;
			for( const auto& rInput : rNode->Inputs )
			{
				bool linked = m_NodeEditor->IsLinked( rInput->ID );
				anyPinsLinked |= linked;

				if( rInput->IsFlagSet( PinFlag_RequiredForEvaluation ) && !linked )
				{
					result.Messages.emplace_back( rNode->ID, rInput->ID, NodeEdPreCompCategory_Standard, NodeEdPreCompError_MissingRequiredLink );

					result.Succeeded = false;
				}
			}

			for( const auto& rOutput : rNode->Outputs )
			{
				bool linked = m_NodeEditor->IsLinked( rOutput->ID );
				anyPinsLinked |= linked;

				if( !linked )
				{
					result.Messages.emplace_back( rNode->ID, rOutput->ID, NodeEdPreCompCategory_Warning, NodeEdPreCompWarning_SkippingNodeWithNotConnectedViaOutput );
				}
			}

			if( !anyPinsLinked )
			{
				result.Messages.emplace_back( 0llu, 0llu, NodeEdPreCompCategory_Warning, NodeEdPreCompWarning_SkippingUnlinkedNode );
			}
		}

		return result;
	}

}
