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

#include "ImGuiWindow.h"

namespace Saturn {

	class RuntimeCommandWindow : public ImGuiWindow
	{
	public:
		RuntimeCommandWindow();
		RuntimeCommandWindow( const std::string& rName );

		virtual ~RuntimeCommandWindow() = default;

	public:
		//////////////////////////////////////////////////////////////////////////
		// ImGuiWindow

		virtual void OnImGuiRender() override;
		virtual void OnUpdate( Timestep ts ) {}
		virtual void OnEvent( Event& rEvent ) {}

		static inline const char* GetStaticName()
		{
			return "Command Window";
		}

	public:
		inline size_t IncrementCmdHistoryIndex() { return ++m_CurrentCommandHistoryIndex; }
		inline size_t DecrementCmdHistoryIndex() { if( m_CurrentCommandHistoryIndex == 0 ) return 0; else return --m_CurrentCommandHistoryIndex; }

		size_t GetCmdHistoryIndex() const { return m_CurrentCommandHistoryIndex; }

		inline void SetCmdHistoryIndex( size_t index ) { m_CurrentCommandHistoryIndex = index; }

		std::vector<std::string>& GetCommandHistory() { return m_CommandHistory; }
		const std::vector<std::string>& GetCommandHistory() const { return m_CommandHistory; }

	private:
		void OnCommandEntered();

	private:
		std::string m_CommandNameBuffer;
		std::vector<std::string> m_CommandHistory;
		size_t m_CurrentCommandHistoryIndex = 0;
	};
		
}
