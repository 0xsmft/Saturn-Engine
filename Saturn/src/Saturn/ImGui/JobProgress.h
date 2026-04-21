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

#include "Saturn/Core/Timer.h"
#include "Saturn/Core/JobSystem.h"

#include <string>
#include <functional>

namespace Saturn {

	class JobProgress : public RefTarget
	{
	public:
		JobProgress() = default;
		~JobProgress() = default;

		float GetProgress() const { return m_Progress.load(); }
		const std::string& GetStatus() const { return m_Status; }
		const std::string& GetTitle() const { return m_Title; }
		bool Completed() const { return m_Done.load(); }

		inline void OnComplete() { m_Done.store( true ); }

		template<typename Func>
		inline void SetJobFunc( Func&& rrFunc ) 
		{
			JobSystem::Get().QueueJob( rrFunc );
		}

		void SetStatus( const std::string& rStatus ) { m_Status = rStatus; }
		void SetTitle( const std::string& rTitle ) { m_Title = rTitle; }

		void SetProgress( float progress ) { m_Progress.store( progress ); }
		void AddProgress( float progress ) { m_Progress += progress; }

		inline void Reset() 
		{
			m_Progress = 0.0f;
			m_Status = "";
			m_Title = "";
		}

	private:
		std::string m_Status;
		std::string m_Title;
		std::atomic<float> m_Progress = 0.0f;
		std::atomic_bool m_Done = false;
	};
}
