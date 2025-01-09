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

#include "SoundSpecification.h"
#include "AudioCore.h"

#include <miniaudio.h>
#include <filesystem>

namespace Saturn {

	class SoundBase : public RefTarget
	{
	public:
		SoundBase() = default;
		virtual ~SoundBase() = default;

		virtual void Play( int frameOffset ) = 0;
		virtual void Stop() = 0;
		virtual void Loop(bool loop = true) = 0;
		virtual void Load( uint32_t flags ) = 0;
		virtual void Reset() = 0;
		virtual void Unload() = 0;

	public:
		ma_sound* GetRawSound() { return m_Sound; }
		const AssetID GetPlayerID() const { return m_PlayerID; }
		inline void SetID( UUID id ) { m_PlayerID = id; }
		inline void MarkForDestroy() { m_MarkedForDestroy = true; }

	protected:
		ma_sound* m_Sound = nullptr;
		Ref<SoundSpecification> m_Specification;

		UUID m_PlayerID = 0;

		bool m_Loaded = false;
		bool m_Playing = false;
		bool m_Looping = false;
		bool m_MarkedForDestroy = false;

	private:
		friend class AudioSystem;
	};
}