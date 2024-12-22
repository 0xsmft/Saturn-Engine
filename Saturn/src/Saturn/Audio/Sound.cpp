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

#include "sppch.h"
#include "Sound.h"

#include "AudioSystem.h"

namespace Saturn {

	Sound::Sound( const Ref<SoundSpecification>& rSpec )
		: SoundBase()
	{
		m_Specification = rSpec;
	}

	void Sound::Load( uint32_t flags )
	{
		if( !m_Loaded )
		{
			SAT_CORE_INFO( "Loading sound: {0}", m_Specification->Name );

			// By default always use the master sound group.
			m_SoundGroup = AudioSystem::Get().GetMasterSoundGroup();

#if defined(SAT_DIST)
			LoadForDist( flags );
#else
			LoadFromFile( flags );
#endif

			m_Sound->pEndCallbackUserData = reinterpret_cast< void* >( static_cast< intptr_t >( m_PlayerID ) );
			m_Sound->endCallback = OnSoundEnd;

			m_Loaded = true;
		}
	}

	void Sound::LoadForDist( uint32_t flags )
	{
#if defined(SAT_DIST)
		auto& rDecodedInformation = m_Specification->DecodedInformation;

		m_Sound = new ma_sound();

		ma_audio_buffer_config bufferConfig = ma_audio_buffer_config_init( (ma_format)rDecodedInformation.Format, rDecodedInformation.Channels, rDecodedInformation.PCMFrameCount, rDecodedInformation.PCMFrames.Data, nullptr );

		MA_CHECK( ma_audio_buffer_init( &bufferConfig, &m_AudioBuffer ) );

		MA_CHECK( ma_sound_init_from_data_source( 
			&AudioSystem::Get().GetAudioEngine(),
			&m_AudioBuffer, flags,
			m_SoundGroup->GetInternal(), m_Sound ) );

//		ma_audio_buffer_uninit( &audioBuffer );

		if( ( flags & ( uint32_t ) MA_SOUND_FLAG_NO_SPATIALIZATION ) == 0 )
			SetupSpatialisation();
#endif
	}

	void Sound::LoadFromFile( uint32_t flags )
	{
		ma_uint32 initFlags = MA_SOUND_FLAG_DECODE | MA_SOUND_FLAG_ASYNC;
		initFlags |= flags;

		m_Sound = new ma_sound();

		// TODO: Wait for the sound to load by using the fence.
		MA_CHECK( ma_sound_init_from_file( &AudioSystem::Get().GetAudioEngine(),
			m_Specification->SoundSourcePath.string().c_str(),
			initFlags, m_SoundGroup->GetInternal(), nullptr, m_Sound ) );

		if( ( initFlags & ( uint32_t ) MA_SOUND_FLAG_NO_SPATIALIZATION ) == 0 )
			SetupSpatialisation();
	}

	void Sound::SetupSpatialisation()
	{
		m_Spatialisation = true;
		SetMinDistance( 1.0f );
		SetMaxDistance( 10.0f );

		ma_sound_set_min_gain( m_Sound, 1.0f );
		ma_sound_set_max_gain( m_Sound, 100.0f );
	}

	void Sound::Unload()
	{
		if( m_Loaded )
		{
			ma_sound_uninit( m_Sound );
			delete m_Sound;

			m_Sound = nullptr;

#if defined( SAT_DIST )
			ma_audio_buffer_uninit( &m_AudioBuffer );
#endif
			m_Loaded = false;
			m_Playing = false;
		}
	}

	Sound::~Sound()
	{
		Stop();
		Unload();
	}

	void Sound::Play( int frameOffset )
	{
		if( !m_Loaded )
			Load( 0 );

		if( ma_sound_at_end( m_Sound ) )
		{
			SAT_CORE_WARN( "Playing sound from beginning: {0}", m_Specification->Name );
			Reset();
		}

		if( frameOffset == 0 )
		{
			SAT_CORE_INFO( "Trying to start sound \"{0}\" now", m_Specification->Name );
			MA_CHECK( ma_sound_start( m_Sound ) );
		}
		else
		{
			SAT_CORE_INFO( "Trying to start sound \"{0}\" in {1} frames", m_Specification->Name, frameOffset );
			ma_sound_set_start_time_in_pcm_frames( m_Sound,
				ma_engine_get_time_in_pcm_frames( &AudioSystem::Get().GetAudioEngine() )
				+ ( ma_engine_get_sample_rate( &AudioSystem::Get().GetAudioEngine() ) * frameOffset ) );

			MA_CHECK( ma_sound_start( m_Sound ) );
		}

		m_Playing = true;
	}

	void Sound::Stop()
	{
		if( !m_Playing )
			return;

		MA_CHECK( ma_sound_stop( m_Sound ) );
		m_Playing = false;
	}

	void Sound::Loop( bool loop )
	{
		ma_sound_set_looping( m_Sound, loop );
		m_Looping = loop;

		if( ma_sound_at_end( m_Sound ) && loop )
		{
			Play();
		}
	}

	bool Sound::IsPlaying() const
	{
		return m_Playing;
	}

	bool Sound::IsLooping() const
	{
		return m_Looping;
	}

	void Sound::WaitUntilLoaded()
	{
		while( !m_Loaded )
		{
			std::this_thread::yield();
		}
	}

	void Sound::Reset()
	{
		if( m_Loaded )
		{
			MA_CHECK( ma_sound_seek_to_pcm_frame( m_Sound, 0 ) );
		}
	}

	void Sound::SetPosition( const glm::vec3& rPos )
	{
		if( m_Spatialisation )
		{
			ma_sound_set_position( m_Sound, rPos.x, rPos.y, rPos.z );
		}
	}

	void Sound::SetSpatialisation( bool value )
	{
		if( m_Spatialisation == value )
			return;

		ma_sound_set_spatialization_enabled( m_Sound, value );
		m_Spatialisation = value;
	}

	void Sound::SetMaxDistance( float dist )
	{
		ma_sound_set_max_distance( m_Sound, dist );
	}

	void Sound::SetMinDistance( float dist )
	{
		ma_sound_set_min_distance( m_Sound, dist );
	}

	void Sound::SetVolumeMultiplier( float multiplier )
	{
		ma_sound_set_volume( m_Sound, multiplier );
	}

	void Sound::SetPitchMultiplier( float multiplier )
	{
		ma_sound_set_pitch( m_Sound, multiplier );
	}

	void Sound::SetPitchByPercent( float percent )
	{
		// If percent was 3.5 multiplier should be 1.035 and if percent was -3.5 then multiplier should be 0.965
		float multiplier = ( percent >= 0.0f ) 
			? 1.0f + ( glm::abs( percent ) / 100.0f ) 
			: 1.0f - ( glm::abs( percent ) / 100.0f );

		SetPitchMultiplier( multiplier );
	}

	void Sound::OnSoundEnd( void* pUserData, ma_sound* pSound )
	{
		UUID ID = static_cast< uint64_t >( reinterpret_cast< intptr_t >( pUserData ) );
		AudioSystem::Get().ReportSoundCompleted( ID );
	}
}