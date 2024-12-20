/********************************************************************************************
*                                                                                           *
*                                                                                           *
*                                                                                           *
* MIT License                                                                               *
*                                                                                           *
* Copyright (c) 2020 - 2024 BEAST                                                           *
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

#include <thread>
#include <functional>
#include <mutex>
#include <condition_variable>

namespace Saturn {

	class GameThread
	{
	public:
		SAT_SINGLETON_LAZY( GameThread )

	public:
		GameThread();
		~GameThread();

		template<typename Fn, typename... Args>
		void Submit( Fn&& rrFunc, Args&&... rrArgs ) 
		{
#if defined(SAT_ENABLE_GAMETHREAD)
			std::unique_lock<std::mutex> Lock( m_Mutex );
			m_Cond.notify_one();

			m_CommandBuffer.push_back( rrFunc );
#else
			rrFunc();
#endif
		}

		void Terminate();

	private:
		void ThreadRun();
	private:
		std::thread m_Thread;
		std::mutex m_Mutex;
		std::condition_variable m_Cond;

		std::vector<std::function<void()>> m_CommandBuffer;

		bool m_ExecuteAll = false;
		std::shared_ptr<std::atomic_bool> m_Running;
	};
}