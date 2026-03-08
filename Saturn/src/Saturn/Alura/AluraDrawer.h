/********************************************************************************************
*                                                                                           *
*                                                                                           *
*                                                                                           *
* MIT License                                                                               *
*                                                                                           *
* Copyright (c) 2026 BEAST                                                                  *
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

#include "Saturn/Core/Event.h"

#include "Saturn/GameFramework/SClass.h"

namespace Saturn {

	SCLASS()
	class AluraDrawer : public SObject
	{
		// NOTE: SAT_DECLARE_CLASS expanded
	private:
		AluraDrawer& operator=( AluraDrawer&& );
		AluraDrawer& operator=( const AluraDrawer& );
		static SClass* GetStaticClassInternal();

	public:
		inline static [[nodiscard]] SClass* StaticClass()
		{
			return GetStaticClassInternal();
		}
	public:
		typedef AluraDrawer ThisClass;
		typedef SObject Super;

	public:
		AluraDrawer() = default;
		~AluraDrawer() = default;

		virtual void OnInit() = 0;
		virtual void OnDraw( Timestep ts ) = 0;
		virtual void OnDestroy() = 0;
		virtual void OnEvent( Event& rEvent ) = 0;
	};
	
}
