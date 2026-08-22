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
#include "AluraLayer.h"

#include "Saturn/Core/Ruby/RubyEvent.h"

#include "AluraCanvas.h"

#include "SharedGlobals.h"

namespace Saturn {

	AluraLayer::AluraLayer()
	{
	}

	AluraLayer::~AluraLayer()
	{
	}

	void AluraLayer::OnEvent( Event& rEvent )
	{
		if( !g_AluraCanvas ) return;

		if( ( rEvent.Category & EC_Ruby ) != 0 )
		{
			if(
				g_AluraCanvas->IsAnyItemHot() ||
				g_AluraCanvas->IsAnyItemActive() ||
				g_AluraCanvas->IsAnyItemFocused() ||
				g_AluraCanvas->IsAnyItemSelected() ||
				g_AluraCanvas->IsAnyRegionHot() )
			{
				switch( rEvent.Type )
				{
					case EventType::MousePressed:
					{
						const RubyMouseEvent& rMouseEvent = ( RubyMouseEvent& ) rEvent;
						g_AluraCanvas->UpdateMouseInputState( rMouseEvent.GetButton(), AluraInputActionState::Pressed );
					} break;

					case EventType::MouseReleased:
					{
						const RubyMouseEvent& rMouseEvent = ( RubyMouseEvent& ) rEvent;
						g_AluraCanvas->UpdateMouseInputState( rMouseEvent.GetButton(), AluraInputActionState::Released );
					} break;

					case EventType::KeyPressed:
					{
						const RubyKeyEvent& rKeyEvent = ( RubyKeyEvent& ) rEvent;
						g_AluraCanvas->UpdateKeyInputState( rKeyEvent.GetKeycode(), AluraInputActionState::Pressed );
					} break;

					case EventType::KeyReleased:
					{
						const RubyKeyEvent& rKeyEvent = ( RubyKeyEvent& ) rEvent;
						g_AluraCanvas->UpdateKeyInputState( rKeyEvent.GetKeycode(), AluraInputActionState::Released );
					} break;

					case EventType::KeyHeld:
					{
						const RubyKeyEvent& rKeyEvent = ( RubyKeyEvent& ) rEvent;
						g_AluraCanvas->UpdateKeyInputState( rKeyEvent.GetKeycode(), AluraInputActionState::Held );
					} break;

					case EventType::InputCharacter:
					{
						const RubyCharacterEvent& rCharEvent = ( RubyCharacterEvent& ) rEvent;
						g_AluraCanvas->UpdateKeyInputState_ForInputText( rCharEvent.GetCharacter() );
					} break;

					case EventType::MouseScroll:
					{
						const RubyMouseScrollEvent& rScrollEvent = ( RubyMouseScrollEvent& ) rEvent;
						g_AluraCanvas->UpdateMouseScroll( { rScrollEvent.GetOffsetX(), rScrollEvent.GetOffsetY() } );
					} break;

					default:
						break;
				}

				// Do not allow any other layer to handle this event.
				rEvent.Handled = true;
				m_AluraWantsControl = true;
			}
			else
			{
				m_AluraWantsControl = false;
			}
		}
	}

}
