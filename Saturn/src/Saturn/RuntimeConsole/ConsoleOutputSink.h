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

#include <string>
#include <vector>

namespace Saturn {

	enum class ConsoleCommandMessageType : uint8_t
	{
		Info,
		Warning,
		Error
	};

	struct ConsoleMessage
	{
		std::string FormattedMessage;
		ConsoleCommandMessageType MessageType = ConsoleCommandMessageType::Info;
	};
	
	class ConsoleOutputSink
	{
	public:
		ConsoleOutputSink();
		~ConsoleOutputSink();

		template<typename... Args>
		void SinkFormatted( const std::format_string<Args...> fmt, ConsoleCommandMessageType type = ConsoleCommandMessageType::Info, Args&&... rrArgs )
		{
			std::string text;
			std::format_to( std::back_inserter( text ), fmt, std::forward< Args >( rrArgs )... );

			Sink( text, type );
		}

		void Sink( const std::string& rMessage, ConsoleCommandMessageType type = ConsoleCommandMessageType::Info );

	public:
		const std::vector<ConsoleMessage>& GetMessages() const { return m_Messages; }

	private:
		std::vector<ConsoleMessage> m_Messages;
	};

}
