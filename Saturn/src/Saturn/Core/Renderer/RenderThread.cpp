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

#include "sppch.h"
#include "RenderThread.h"

#include "Saturn/Core/OptickProfiler.h"

namespace Saturn {

	RenderThread::RenderThread()
		: Thread()
	{
	}

	RenderThread::~RenderThread()
	{
	}

	void RenderThread::Start()
	{
		if( !m_Enabled )
			return;

		m_Running->store( m_Enabled );
		m_Thread = std::thread( &RenderThread::ThreadRun, this );
	}

	void RenderThread::RequestJoin()
	{
		std::lock_guard<std::mutex> Lock( m_Mutex );

		m_Running->store( false );
		m_SignalCV.notify_one();
	}

	void RenderThread::WaitAll()
	{
		SAT_PF_EVENT();

		m_WaitTime.Reset();

		// If we are not using the render thread, we still need to execute the command buffer. 
		// So just do it the now and return out.
		if( !m_Enabled ) 
		{
			ExecuteCommands();

			m_WaitTime.Stop();

			return;
		}

		if( !m_CommandBuffer.empty() )
		{
			m_ExecuteAll = true;
			m_SignalCV.notify_one();
		}

		WaitCommands();

		m_WaitTime.Stop();
	}

	void RenderThread::ExecuteOne()
	{
		m_ExecuteOne = true;
		m_SignalCV.notify_one();
	}

	bool RenderThread::IsRenderThread()
	{
		return std::this_thread::get_id() == m_ThreadID;
	}

	void RenderThread::ThreadRun()
	{
		SetThreadDescription( GetCurrentThread(), L"Render Thread" );
		m_ThreadID = std::this_thread::get_id();

		while (true)
		{
			SAT_PF_THRD( "Render Thread" );

			std::unique_lock<std::mutex> Lock( m_Mutex );

			// m_SignalCV = What do we want to do, ExecuteOne, ExecuteAll
			// m_QueueCV  = What state is the queue in, empty or not empty
			// Every time one of the two has changed we must notify it

			// Wait for main thread signal
			m_SignalCV.wait( Lock, [this] 
				{
					return !m_Running->load() || m_ExecuteAll || m_ExecuteOne;
				} );

			// Wait for the queue to not be empty.
			m_QueueCV.wait( Lock, [this]
				{
					return !m_Running->load() || !m_CommandBuffer.empty();
				} );

			if( !m_Running->load() ) break;

			Lock.unlock();

			if( m_ExecuteOne )
			{
				auto& rFunc = m_CommandBuffer.back();

				rFunc();

				m_CommandBuffer.pop_back();

				m_ExecuteOne = false;
			}

			if( m_ExecuteAll )
			{
				ExecuteCommands();

				m_ExecuteAll = false;
			}

			// Tell the main thread we're done.
			m_QueueCV.notify_one();
		}

		m_Running->store( false );
	}

}