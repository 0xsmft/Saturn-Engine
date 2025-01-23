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

#include <miniaudio.h>
#include <string>

namespace Saturn {

	class SoundGroup : public RefTarget
	{
	public:
		SoundGroup();
		SoundGroup( const std::string& rName );
		virtual ~SoundGroup();

		void Init( bool isMaster );
		void Destroy();

		const std::string& GetName() { return m_Name; }
		const std::string& GetName() const { return m_Name; }

		void SetName( const std::string& rName ) { m_Name = rName; }

		float GetVolume() const;
		float GetPitch() const;

		void SetVolume( float volume );
		void SetPitch( float pitch );

		void Start();
		void Stop();

		ma_sound_group* GetInternal() const { return m_Group; }

	private:
		ma_sound_group* m_Group = nullptr;
		std::string m_Name;

	private:
		friend class AudioSystem;
	};
}