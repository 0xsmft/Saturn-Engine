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

#include "SoundBase.h"

namespace Saturn {

	class Sound : public SoundBase
	{
	public:
		Sound( const Ref<SoundSpecification>& rSpec );
		virtual ~Sound();

		virtual void Play( int frameOffset = 0 ) override;
		virtual void Stop() override;
		virtual void Loop( bool loop = true ) override;
		virtual void Load( uint32_t flags = 0 ) override;
		virtual void Unload() override;

		bool IsPlaying() const;
		bool IsLooping() const;

		void WaitUntilLoaded();
		virtual void Reset() override;

		void SetPosition( const glm::vec3& rPos );
		void SetSpatialisation( bool value );
	
		void SetMaxDistance( float dist );
		void SetMinDistance( float dist );

		void SetVolumeMultiplier( float multiplier );
		void SetPitchMultiplier( float multiplier );

		void SetPitchByPercent( float percent );

		float GetDurationInSeconds();
		float GetCursorInSeconds();

		std::string FormatSeconds( float seconds );

		uint64_t GetDurationInPCM();
		uint64_t GetCursorInPCM();

		void SeekTo( uint64_t pcmFrame );

	private:
		static void OnSoundEnd( void* pUserData, ma_sound* pSound );

	private:
		void SetupSpatialisation();
		void LoadForDist( uint32_t flags );
		void LoadFromFile( uint32_t flags );

		Ref<SoundGroup> m_SoundGroup;

		bool m_Spatialisation = false;

#if defined( SAT_DIST )
		ma_audio_buffer m_AudioBuffer;
#endif

	private:
		friend class AudioSystem;
	};
}