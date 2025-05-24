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

#include "Saturn/Core/Base.h"
#include "Saturn/Core/Ref.h"
#include "Saturn/Core/Ruby/RubyEvent.h"

namespace Saturn {

	class ImGuiWindow : public RefTarget
	{
	public:
		ImGuiWindow() = default;
		ImGuiWindow( const std::string& rName ) { m_Name = rName; }
		virtual ~ImGuiWindow() = default;

		virtual void OnImGuiRender() = 0;
		virtual void OnUpdate( Timestep ts ) = 0;
		virtual void OnEvent( RubyEvent& rEvent ) = 0;

		bool IsOpen() const { return m_Open; }
		inline void OpenWindow() { m_Open = true; }
		inline void CloseWindow() { m_Open = false; }
		inline void ShowOrHide() { if( m_Open ) CloseWindow(); else OpenWindow(); }

		const std::string& GetWindowName() { return m_Name; }
		const std::string& GetWindowName() const { return m_Name; }

	protected:
		std::string m_Name = "";
		bool m_Open = false;
	};

}