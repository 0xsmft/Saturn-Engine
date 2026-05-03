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

#include "Saturn/Core/UUID.h"

#include <string>
#include <vector>

namespace Saturn {

	enum class NodeEditorMessageSeverity : uint8_t 
	{
		Info,
		Warning,
		Error
	};

	struct NodeEditorMessage
	{
		std::string MessageText;
		UUID ID;
		NodeEditorMessageSeverity Type = NodeEditorMessageSeverity::Info;
	};

	class NodeEditorOutput
	{
	public:
		NodeEditorOutput( UUID outputWindowID );
		~NodeEditorOutput();

		void Draw();
		void ClearOutput();
		void PushMessage( const NodeEditorMessage& rMessageData );

		[[nodiscard]] bool IsOpen() const { return m_ShowWindow; }

		inline void ShowOrHide() { m_ShowWindow ^= 1; }
		inline void Hide() { m_ShowWindow = false; }
		inline void Show() { m_ShowWindow = true; }

	private:
		void DrawMessage( const NodeEditorMessage& rMessage );
		void ClearMessage( UUID messageID );

	private:
		std::string m_WindowName{};
		std::vector<NodeEditorMessage> m_Messages;
		UUID m_SelectedMessageID = 0;
		UUID m_OutputWindowID;
		bool m_ShowWindow = true;
	};
}
