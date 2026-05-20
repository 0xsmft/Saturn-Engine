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

#include "sppch.h"
#include "Thread.h"

namespace Saturn {

	Thread::Thread()
		: m_Running( std::make_shared<std::atomic_bool>() )
	{
	}

	void Thread::Terminate()
	{
		// TODO: Check what thread is calling this
		//       Probably don't want our thread calling this most likely called from the main thread.

		if( m_Thread.joinable() )
		{
			{
				std::lock_guard<std::mutex> Lock( m_Mutex );
				m_Running->store( false );

				m_QueueCV.notify_all();
				m_SignalCV.notify_all();
			}

			// Wait until our thread has done it's work.
			while( m_Running->load() )
			{
				std::this_thread::yield();
			}

			m_Thread.join();
		}
	}

	Thread::~Thread()
	{
		Terminate();

		m_CommandBuffer.clear();
	}

	void Thread::ExecuteCommands()
	{
		while( !m_CommandBuffer.empty() )
		{
			std::function<void()> currentFunction = nullptr;

			m_Mutex.lock();

			for( auto Itr = m_CommandBuffer.begin(); Itr != m_CommandBuffer.end(); ++Itr )
			{
				auto& rFunc = *Itr;

				if( rFunc )
				{
					currentFunction = rFunc;
				}

				// Make sure we remove it so that it cannot be executed again.
				m_CommandBuffer.erase( Itr );
				break;
			}

			m_Mutex.unlock();

			if( currentFunction )
			{
				currentFunction();
			}
		}
	}

	void Thread::WaitCommands() 
	{
		std::unique_lock<std::mutex> Lock( m_Mutex );
		m_QueueCV.wait( Lock, [=] { return m_CommandBuffer.empty(); } );
		Lock.unlock();
	}

}