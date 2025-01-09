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

#include "Saturn/Core/Ref.h"

#include <thread>
#include <functional>
#include <mutex>

namespace Saturn {

	// RAII Safe.
	class Thread : public RefTarget
	{
	public:
		Thread();
		virtual ~Thread();

		template<typename Fn, typename... Args>
		void Queue( Fn&& rrFunc, Args&&... rrArgs )
		{
			std::unique_lock<std::mutex> Lock( m_Mutex, std::try_to_lock );
			m_QueueCV.notify_one();

			m_CommandBuffer.push_back( std::move( rrFunc ) );
		}

		void Signal() { return m_SignalCV.notify_one(); }

		virtual void Start() = 0;
		virtual void RequestJoin() = 0;

		bool IsCurrentThread() const { return m_ThreadID == std::this_thread::get_id(); }

	protected:
		void ExecuteCommands();
		void WaitCommands();
		void Terminate();

	protected:
		std::thread m_Thread;
		std::thread::id m_ThreadID;
		std::mutex m_Mutex;
		std::shared_ptr<std::atomic_bool> m_Running;

		// What state is the queue in, empty or not empty.
		std::condition_variable m_QueueCV;

		// What do we want to do, ExecuteOne, ExecuteAll or are we even allowed to continue?
		std::condition_variable m_SignalCV;

		std::vector<std::function<void()>> m_CommandBuffer;
	};
}