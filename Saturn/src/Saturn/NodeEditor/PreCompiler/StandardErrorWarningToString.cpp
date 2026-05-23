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

#if !defined(SAT_COMPILER_MSVC)
#include "Saturn/Core/UUID.h"
#endif

#include "NodeEditorPreCompilerBase.h"
#include "StandardErrorWarningToString.h"

namespace Saturn::Auxiliary {
	
	static std::string NodeEditorPreCompStdWarningToStr( uint32_t ec )
	{
		switch( ec )
		{
			case NodeEdPreCompWarning_SkippingUnlinkedNode:
			{
				return "warn STD0x01: Skipping a node with no pins linked.";
			}

			case NodeEdPreCompWarning_SkippingNodeWithNotConnectedViaOutput:
			{
				return "warn STD0x02: Skipping a node with it's output pin not linked.";
			}

			default:
				return "warn STD0x??: Unknown warning.";
		}
	}

	static std::string NodeEditorPreCompStdErrorToStr( uint32_t ec )
	{
		switch( ec )
		{
			case NodeEdPreCompError_InternalError:
			{
				return "error STD0x01: An internal error has occurred.";
			}

			case NodeEdPreCompError_MissingRequiredLink:
			{
				return "error STD0x02: A link is required for a pin.";
			}

			case NodeEdPreCompError_MissingRequiredData:
			{
				return "error STD0x04: A data input is required for a pin.";
			}

			default:
				return "error STD0x??: Unknown error.";
		}
	}
	
	std::string NodeEditorPreCompResultToString( const NodeEditorPreCompileMessage& rMessage )
	{
		if( ( rMessage.Category & NodeEdPreCompCategory_Warning ) != 0 )
		{
			return NodeEditorPreCompStdWarningToStr( rMessage.MessageCode );
		}
		else if( ( rMessage.Category & NodeEdPreCompCategory_Standard ) != 0 )
		{
			return NodeEditorPreCompStdErrorToStr( rMessage.MessageCode );
		}
		else
			return "Unknown message category!";
	}
}
