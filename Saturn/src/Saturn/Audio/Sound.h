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

#include "SoundBase.h"
#include "SoundGroup.h"

namespace Saturn {

	// Sound based from SoundBase
	// 
	// Represents a SoundSpecificationAsset that can be played, stopped and looped.
	// Sound must be created from a SoundSpecificationAsset
	// Automatic registration with the Audio system is provided.
	//
	// Furthermore, this class provides an interface for spatialisation, volume and pitch adjustments, seeking within an audio stream and Sound Groups. 
	//
	class Sound : public SoundBase
	{
	public:
		Sound( const Ref<SoundSpecification>& rSpec, Ref<SoundGroup> soundGroup );
		virtual ~Sound();

	public:
		//////////////////////////////////////////////////////////////////////////
		// SoundBase overrides
		//////////////////////////////////////////////////////////////////////////

		// Play from the current PCM frame or start from beginning if the sound is at the end.
		// You must call SoundBase::Load() BEFORE calling this function!
		// @param frameOffset -- The PCM frame offset which this sound should start from.
		virtual void Play( int frameOffset = 0 ) override;

		virtual void Stop() override;
		virtual void Loop( bool loop = true ) override;

		// Load the actual sound from a data source
		// 
		// Data source could be a file or in Dist, it could be from a compressed sound buffer
		// 
		// This function should only be called if this Sound was not registered with the Audio System.
		// 
		// @param flags -- Load Flags (see: ma_sound_flags) default is no flags
		virtual void Load( uint32_t flags = 0 ) override;

		// Unload the data source
		// You must not call any other functions that require a data source
		//
		// This function should only be called if this Sound was not registered with the Audio System.
		virtual void Unload() override;

		// Set the data source to be at the first PCM frame
		virtual void Reset() override;

		virtual void OnSoundCompleted() override;

	public:
		bool IsLooping() const;

		// Pauses the active thread until the Audio System has fully initialised the data source.
		void WaitUntilLoaded();

		// Set Spatialisation position in world space.
		void SetPosition( const glm::vec3& rPos );

		void SetSpatialisation( bool value );
	
		// Set Spatialisation min or max distance from primary listener.
		void SetMaxDistance( float dist );
		void SetMinDistance( float dist );

		void SetVolume( float volume );
		void SetPitch( float pitch );

		float GetVolume();
		float GetPitch();

		void SetPitchByPercent( float percent );

		float GetDurationInSeconds();
		float GetCursorInSeconds();

		// @returns in formatted string HH:MM:SS
		std::string FormatSeconds( float seconds );

		uint64_t GetDurationInPCM();
		uint64_t GetCursorInPCM();

		// Set the data source to a PCM frame
		void SeekTo( uint64_t pcmFrame );

		void AddOnCompleteFunction( std::function<void(UUID)>&& rrFunc ) { m_CompletionFunctions.emplace_back( rrFunc ); }

	private:
		static void OnSoundEnd( void* pUserData, ma_sound* pSound );

	private:
		void SetupSpatialisation();

		// Load via buffer
		void LoadForDist( uint32_t flags );
		
		// Load via file
		void LoadFromFile( uint32_t flags );

		Ref<SoundGroup> m_SoundGroup;

		bool m_Spatialisation = false;

#if defined( SAT_DIST )
		ma_audio_buffer m_AudioBuffer;
#endif

		std::vector<std::function<void(UUID)>> m_CompletionFunctions;

	private:
		friend class AudioSystem;
	};
}