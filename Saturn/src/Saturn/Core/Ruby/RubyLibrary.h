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

#include "RubyMonitor.h"
#include "RubyEventType.h"

#include "SingletonStorage.h"

#include <unordered_set>

namespace Saturn {

	class RubyLibrary final
	{
	public:
		SAT_SINGLETON_LAZY( RubyLibrary )

	public:
		RubyLibrary();
		~RubyLibrary() = default;

		static void PollEvents();

		std::vector<RubyMonitor> GetAllMonitors();
		RubyMonitor& GetPrimaryMonitor();

		void AddMonintor( const RubyMonitor& rMonitor );

		bool IsKeyDown( RubyKey key )
		{
			return m_Keys.count( key ) > 0;
		}

		bool IsMouseButtonDown( RubyMouseButton btn )
		{
			return m_CurrentMouseButtons.count( btn ) > 0;
		}

		std::unordered_set<RubyMouseButton>& GetCurrentMouseButtons() { return m_CurrentMouseButtons; }
		const std::unordered_set<RubyMouseButton>& GetCurrentMouseButtons() const { return m_CurrentMouseButtons; }

		std::unordered_set<RubyKey>& GetKeys() { return m_Keys; }
		const std::unordered_set<RubyKey>& GetKeys() const { return m_Keys; }

		//////////////////////////////////////////////////////////////////////////

		void SetKeyState( RubyKey key, bool state ) 
		{
			if( state )
				m_Keys.insert( key );
			else
				m_Keys.erase( key );
		}

		void IntrnlSetMouseState( RubyMouseButton btn, bool state )
		{
			if( state )
				m_CurrentMouseButtons.insert( btn );
			else
				m_CurrentMouseButtons.erase( btn );
		}

		void IntrnlClearKeysAndMouse()
		{
			m_Keys.clear();
			m_CurrentMouseButtons.clear();
		}

	private:
		std::unordered_set<RubyKey> m_Keys;
		std::unordered_set<RubyMouseButton> m_CurrentMouseButtons;
		std::vector<RubyMonitor> m_Monitors;
	};
}