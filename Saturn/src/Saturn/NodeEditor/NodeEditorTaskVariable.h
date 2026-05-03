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

#include "NodeEditorVariable.h"

namespace Saturn {

	class NodeEditorTaskVariable
	{
	public:
		NodeEditorTaskVariable() = default;
		~NodeEditorTaskVariable() = default;

		template<typename TCppType>
		typename const TCppType Get() const
		{
			if( !std::holds_alternative<std::monostate>( m_Value ) )
			{
				return std::get<TCppType>( m_Value );
			}

			return TCppType{};
		}

		template<typename TCppType>
		typename TCppType* GetIf()
		{
			if( !std::holds_alternative<std::monostate>( m_Value ) )
			{
				return std::get_if<TCppType>( &m_Value );
			}

			return nullptr;
		}

		template<typename TCppType>
		void WriteValue( TCppType value )
		{
			m_Value = value;
		}

	private:
		NodeEditorVariableTypes m_Value;
	};

}
