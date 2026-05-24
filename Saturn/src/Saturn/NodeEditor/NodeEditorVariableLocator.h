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

namespace Saturn {

	//
	// Pointer to a variable
	// 
	// If NodeEditorVariableLocator owns the variable then it will be allocated on the heap and this class will delete it when it goes out of scope.
	//
	class NodeEditorVariableLocator
	{
	public:
		NodeEditorVariableLocator() = default;
		NodeEditorVariableLocator( const void* pAddress ) 
			: m_pVariable( pAddress )
		{
		}

		~NodeEditorVariableLocator() 
		{
			if( m_Owned )
			{
				DeleteOwned();
			}

			m_Owned = false;
			m_pVariable = nullptr;
		}

		template<typename TCppType>
		void Set( const TCppType* pAddress )
		{
			Reset();

			m_pVariable = pAddress;
		}

		template<typename TCppType>
		void SetOwned( TCppType* pPtr ) 
		{
			Reset();

			m_pVariable = pPtr;
			m_Owned = true;

			m_pDeleterFunc = []( const void* pObj )
			{
				delete static_cast< const TCppType* >( pObj );
			};
		}

		const void* Get() const { return m_pVariable; }

	private:
		void Reset() 
		{
			if( m_pVariable && m_Owned )
			{
				DeleteOwned();
			}
		}

		void DeleteOwned() 
		{
			if( m_pDeleterFunc )
			{
				( m_pDeleterFunc ) ( m_pVariable );
			}
		}

	private:
		const void* m_pVariable = nullptr;
		void ( *m_pDeleterFunc )( const void* ) = nullptr;
		
		bool m_Owned = false;
	};
	
}
