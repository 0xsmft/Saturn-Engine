/********************************************************************************************
*                                                                                           *
*                                                                                           *
*                                                                                           *
* MIT License                                                                               *
*                                                                                           *
* Copyright (c) 2020 - 2025 BEAST                                                           *
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
#include "AnimGraphAnimationPin.h"

#include "AnimGraph.h"
#include "AnimGraphStateMachinePlayerNode.h"

#include "Saturn/NodeEditor/NodeEditorNodeBase.h"

#include "imgui.h"

namespace Saturn {

	AnimGraphAnimationPin::AnimGraphAnimationPin( const std::string& rName, PinKind kind, AnimGraphAnimationPinFlags flags )
		: Pin( rName, PinType::AnimGraphAnimation, kind ), m_Flags( flags )
	{
	}

	AnimGraphAnimationPin::AnimGraphAnimationPin( UUID id, const std::string& rName, PinType type, UUID nodeID )
		: Pin( id, rName, type, nodeID )
	{
	}

	AnimGraphAnimationPin::~AnimGraphAnimationPin()
	{
	}

	void AnimGraphAnimationPin::Serialise( std::ofstream& rStream ) const
	{
	}

	void AnimGraphAnimationPin::Deserialise( FDependentIStream& rStream )
	{
	}

	void AnimGraphAnimationPin::OnRenderOutput()
	{
		switch( m_Flags )
		{
			case AnimGraphAnimationPinFlags::StateMachine:
			{
				if( ImGui::Button( "Open State Machine" ) ) 
				{
					auto AG = dynamic_cast<AnimGraph*>( Node->pOuter );
					if( AG )
					{
						auto playerNode = dynamic_cast< AnimGraphStateMachinePlayerNode* >( Node.Get() );
						AG->ChangeViewMode( AnimGraphViewMode::StateMachine );
					}
				}
			} break;

			case AnimGraphAnimationPinFlags::Animation:
			default:
				break;
		}
	}

	void AnimGraphAnimationPin::OnRenderInput()
	{
	}

}
