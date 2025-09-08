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

#pragma once

#include "Saturn/NodeEditor/Pin.h"

namespace Saturn {

	enum class AnimGraphAnimationPinFlags 
	{
		// Does this pin input/output a single animation
		Animation,
		// or does it input/output a state machine
		StateMachine
	};

	class AnimGraphAnimationPin : public Pin
	{
	public:
		AnimGraphAnimationPin() = default;
		AnimGraphAnimationPin( const std::string& rName, PinKind kind, AnimGraphAnimationPinFlags flags );
		AnimGraphAnimationPin( UUID id, const std::string& rName, PinType type, UUID nodeID );
		
		~AnimGraphAnimationPin();

	public:
		virtual void Serialise( std::ofstream& rStream ) const override;
		virtual void Deserialise( FDependentIStream& rStream ) override;

	protected:
		virtual void OnRenderOutput() override;
		virtual void OnRenderInput() override;

	private:
		AnimGraphAnimationPinFlags m_Flags = AnimGraphAnimationPinFlags::Animation;
	};

}
