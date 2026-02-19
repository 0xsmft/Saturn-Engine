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

#include "Saturn/Core/Thread.h"
#include "SingletonStorage.h"

namespace Saturn {

	class RenderThread : public Thread
	{
	public:
		SAT_SINGLETON_LAZY( RenderThread )

	public:
		RenderThread();
		virtual ~RenderThread();

		void WaitAll();

		// Executes the most recent command.
		void ExecuteOne();

		float GetWaitTime() { return m_WaitTime.ElapsedMilliseconds(); }

		bool IsRenderThread();

		void EnableIf( bool enable ) { m_Enabled = enable; }

	public:
		virtual void Start() override;
		virtual void RequestJoin() override;

	private:
		void ThreadRun();

	private:
		bool m_ExecuteAll = false;
		bool m_ExecuteOne = false;
		bool m_Enabled = false;

		Timer m_WaitTime;
	};
}