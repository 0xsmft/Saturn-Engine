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

#include <iostream>
#include <string>
#include <chrono>
#include <unordered_map>
#include <string>

namespace Saturn {

	class Timer
	{
	public:
		Timer()
		{
			Reset();
		}

		void Stop() 
		{
			m_Stop = std::chrono::high_resolution_clock::now();
			m_Stopped = true;
		}

		void Reset() 
		{
			m_Start = std::chrono::high_resolution_clock::now();
			m_Stopped = false;
		}
		
		float Elapsed() const
		{
			if( m_Stopped )
				return std::chrono::duration< float, std::milli >( m_Stop - m_Start ).count();
			else
				return std::chrono::duration< float, std::milli >( std::chrono::high_resolution_clock::now() - m_Start ).count();
		}
		
		float ElapsedMilliseconds() const
		{
			return Elapsed();
		}

	private:
		bool m_Stopped = false;

		std::chrono::time_point<std::chrono::high_resolution_clock> m_Start;
		std::chrono::time_point<std::chrono::high_resolution_clock> m_Stop;
	};

	class TimerOnEvent
	{
	public:
		TimerOnEvent() {}

		// Time in seconds
		template<typename Fn, typename... Args>
		TimerOnEvent( float Interval, bool repeat, Fn&& rrFunction, Args&&... rrArgs )
			: m_Function(std::bind(std::forward<Fn>(rrFunction), std::forward<Args>(rrArgs)...))
		{
			m_Interval = Interval;
			m_Repeat = repeat;

			Reset();
		}
		
		~TimerOnEvent() 
		{
			ForceStop();
		}

		TimerOnEvent( const TimerOnEvent& ) = delete;
		TimerOnEvent& operator=( const TimerOnEvent& ) = delete;

		void Start()
		{
			m_FuncThread = std::thread( [this]() 
				{
					while (!ShouldStop())
					{
						// Call the function
						std::invoke(m_Function);

						std::this_thread::sleep_for( std::chrono::seconds( ( int64_t ) m_Interval ) );

						Reset();
					}
				} );
		}

		void Reset() 
		{
			m_Start = std::chrono::high_resolution_clock::now();

			m_EndTime = m_Start + std::chrono::seconds( (int64_t)m_Interval );
		}

		void ForceStop() 
		{
			if( m_FuncThread.joinable() )
				m_FuncThread.join();
		}
	private:
		bool ShouldStop() 
		{
			auto timeNow = std::chrono::high_resolution_clock::now();

			if( timeNow == m_EndTime && !( m_Repeat ) )
				return true;
			else
				return false;
		}

	private:
		float m_Interval = 0.0f;
		bool m_Repeat = false;

		std::chrono::time_point<std::chrono::high_resolution_clock> m_Start;
		std::chrono::time_point<std::chrono::high_resolution_clock> m_EndTime;

		std::function<void()> m_Function;

		std::thread m_FuncThread;
	};

	using TimerMap = std::unordered_map<std::string, Timer>;
}