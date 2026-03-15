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
#include "AudioSystem.h"

#include "SoundGroup.h"

#include "Saturn/Asset/AssetManager.h"
#include "Saturn/Project/Project.h"
#include "Saturn/Core/Profiler.h"

namespace Saturn {

	//////////////////////////////////////////////////////////////////////////

	AudioThread::AudioThread()
		: Thread()
	{
	}

	AudioThread::~AudioThread()
	{
	}

	void AudioThread::Start()
	{
		m_Running.store( true );
		m_Thread = std::thread( &AudioThread::ThreadRun, this );
	}

	void AudioThread::RequestJoin()
	{
		std::unique_lock<std::mutex> Lock( m_Mutex );
		
		m_Running.store( false );
		m_SignalCV.notify_one();
		
		Lock.unlock();
	}

	void AudioThread::ThreadRun()
	{
#if defined(SAT_PLATFORM_WINDOWS)
		::SetThreadDescription( ::GetCurrentThread(), L"Audio Thread" );
#endif
		m_ThreadID = std::this_thread::get_id();

		while( true )
		{
			std::unique_lock<std::mutex> Lock( m_Mutex );
		
			// Wait for the queue to not be empty.
			m_QueueCV.wait( Lock, [this]
				{
					return !m_Running.load() || !m_CommandBuffer.empty();
				} );

			if( !m_Running.load() ) break;

			Lock.unlock();

			ExecuteCommands();

			m_QueueCV.notify_all();
		}

		m_Running.store( false );
	}

	//////////////////////////////////////////////////////////////////////////

	AudioSystem::AudioSystem()
	{
		Initialise();
	}

	void AudioSystem::Initialise()
	{
		SAT_CORE_INFO( "[AudioSystem::Initialise]... Allocating Engine, Device and Context.");
		m_pContext = new ma_context();
		m_pDevice = new ma_device();
		m_pEngine = new ma_engine();

		SAT_CORE_INFO( "Starting Audio Thread...");
		m_AudioThread = Ref<AudioThread>::Create();
		m_AudioThread->Start();

		// Queue initialisation on newly created audio thread.
		m_AudioThread->Queue( 
			[&]() 
			{
				// Create engine
				MA_CHECK( ma_engine_init( nullptr, m_pEngine ) );

				// Init context.
				ma_backend backends[ 1 ] = { ma_backend_wasapi };
				MA_CHECK( ma_context_init( backends, 1, nullptr, m_pContext ) );

				ma_device_info deviceInfo;
				MA_CHECK( ma_context_get_device_info( m_pContext, ma_device_type_playback, nullptr, &deviceInfo ) );

				ma_device_config deviceConfig = ma_device_config_init( ma_device_type_playback );
				deviceConfig.playback.format = ma_format_f32;
				deviceConfig.playback.channels = 2;
				deviceConfig.sampleRate = 48000;
				deviceConfig.dataCallback = nullptr;

				// Init device.
				MA_CHECK( ma_device_init( m_pContext, &deviceConfig, m_pDevice ) );

				SAT_CORE_INFO( "Audio Device information:" );
				SAT_CORE_INFO( " Using backend API: {0}", ma_get_backend_name( backends[ 0 ] ) );
				SAT_CORE_INFO( " Device Name: {0}", deviceInfo.name );
				SAT_CORE_INFO( " Is Primary: {0}", deviceInfo.isDefault );
				SAT_CORE_INFO( " Channels: {0}", m_pDevice->playback.channels );
				SAT_CORE_INFO( " Sample Rate: {0}", m_pDevice->playback.internalSampleRate );
				SAT_CORE_INFO( " Buffer Cap: {0}", m_pDevice->playback.intermediaryBufferCap );
				SAT_CORE_INFO( " Format: {0}", ( uint32_t )m_pDevice->playback.format );
				SAT_CORE_INFO( "==============" );

				m_MasterSoundGroup = Ref<SoundGroup>::Create( "Master" );
				m_MasterSoundGroup->Init( true );

				m_Initialised.store( true );
			} );
	}

	void AudioSystem::Terminate()
	{
		// Stop and unload any alive sounds
		for( auto& [id, sound] : m_AliveSounds )
		{
			sound->Stop();
			sound->Unload();
		}

		// Unload remaining sounds
		for( auto& [id, sound] : m_LoadedSounds )
		{
			sound->Unload();
		}

		if( Ref<Project> activeProject = Project::GetActiveProject(); activeProject != nullptr )
		{
			// Uninit project sound groups
			for( auto& rGroup : activeProject->GetSoundGroups() )
			{
				rGroup->Destroy();
			}
		}

		// And the master sound group as well
		m_MasterSoundGroup->Destroy();
		m_MasterSoundGroup = nullptr;

		// Stop audio thread
		SAT_CORE_INFO( "Stopping Audio Thread..." );
		m_AudioThread->RequestJoin();

		MA_CHECK( ma_device_stop( m_pDevice ) );
		MA_CHECK( ma_context_uninit( m_pContext ) );

		// NOTE: Device is owned by the engine, so it will uninit it for us.
		ma_engine_uninit( m_pEngine );

		delete m_pEngine;
		delete m_pDevice;
		delete m_pContext;

		m_AliveSounds.clear();
		m_LoadedSounds.clear();
	}

	void AudioSystem::WaitForInit()
	{
		while( !m_Initialised.load() )
		{
			std::this_thread::yield();
		}
	}

	AudioSystem::~AudioSystem()
	{
		// No cleanup done in destructor, application already terminated the audio system.
	}

	void AudioSystem::PlaySound( Ref<Sound> soundAsset )
	{
		// TODO: Check if we are on the audio thread.
		soundAsset->Play();
		m_AliveSounds[ soundAsset->GetPlayerID() ] = soundAsset;
	}

	Ref<Sound> AudioSystem::RequestNewSound( AssetID ID, UUID UniquePlayerID, bool PlayNow /*= true */, Ref<SoundGroup> soundGroup /*= nullptr*/ )
	{
		// Load the sound spec.
		Ref<SoundSpecification> spec = AssetManager::Get()->GetAssetAs<SoundSpecification>( ID );

		Ref<Sound> newSound = Ref<Sound>::Create( spec, soundGroup );
		newSound->SetID( UniquePlayerID );
		newSound->MarkSet( 1u );
		m_AliveSounds[ UniquePlayerID ] = newSound;

		auto loadFunc = [=]() -> void
		{
			// Intentional.
			// Better to get the sound again rather than copy it into this lambda.
			Ref<Sound> newSound = m_AliveSounds[ UniquePlayerID ];

			newSound->Load( MA_SOUND_FLAG_NO_SPATIALIZATION );
			// If the sound was already loaded then we can still disable it here.
			newSound->SetSpatialisation( false );

			if( PlayNow ) newSound->Play();

			m_LoadedSounds[ UniquePlayerID ] = newSound;
		};

		m_AudioThread->IsCurrentThread() ? loadFunc() : m_AudioThread->Queue( loadFunc );

		return newSound;
	}

	void AudioSystem::FireAndForget( const std::string& rAssetName )
	{
		auto asset = AssetManager::Get()->FindAsset( rAssetName, AssetType::Sound );
		if( asset )
		{
			FireAndForget( asset->ID );
		}
	}

	void AudioSystem::FireAndForget( AssetID id )
	{
		RequestNewSound( id, UUID() );
	}

	Ref<Sound> AudioSystem::PlaySoundAtLocation( AssetID ID, UUID UniquePlayerID, const glm::vec3& rPos, bool PlayNow /*= true */, Ref<SoundGroup> soundGroup /* = nullptr */ )
	{
		// Load the sound spec.
		Ref<SoundSpecification> spec = AssetManager::Get()->GetAssetAs<SoundSpecification>( ID );

		Ref<Sound> newSound = Ref<Sound>::Create( spec, soundGroup );
		newSound->SetID( UniquePlayerID );
		newSound->MarkSet( 1u );

		m_AliveSounds[ UniquePlayerID ] = newSound;

		auto loadFunc = [=]() -> void
		{
			// Intentional.
			// Better to get the sound again rather than copy it into this lambda.
			Ref<Sound> newSound = m_AliveSounds[ UniquePlayerID ];

			newSound->Load();
			// If the sound was already loaded then we can still enable it here.
			newSound->SetSpatialisation( true );
			newSound->SetPosition( rPos );

			if( PlayNow ) newSound->Play();

			m_LoadedSounds[ UniquePlayerID ] = newSound;
		};

		m_AudioThread->IsCurrentThread() ? loadFunc() : m_AudioThread->Queue( loadFunc );

		return newSound;
	}

	Ref<Sound> AudioSystem::RequestPreviewSound( AssetID ID, UUID UniquePlayerID /*= UUID()*/, bool PlayNow /*= true*/, Ref<SoundGroup> soundGroup /*= nullptr*/ )
	{
		return nullptr;
	}

	Ref<GraphSound> AudioSystem::PlayGraphSound( AssetID ID, UUID UniquePlayerID, bool spatialisation /*=false*/, bool PlayNow /*= false*/ )
	{
		Ref<GraphSound> snd = Ref<GraphSound>::Create( ID );
		snd->SetID( UniquePlayerID );
		snd->MarkSet( 1u );

		m_AliveSounds[ UniquePlayerID ] = snd;

		auto loadFunc = [=]()
			{
				// Intentional.
				Ref<GraphSound> graphSoundAsset = m_AliveSounds[ UniquePlayerID ];

				graphSoundAsset->Load( !spatialisation ? MA_SOUND_FLAG_NO_SPATIALIZATION : 0 );
				
				if( PlayNow ) graphSoundAsset->Play();

				m_LoadedSounds[ UniquePlayerID ] = graphSoundAsset;
			};

		m_AudioThread->IsCurrentThread() ? loadFunc() : m_AudioThread->Queue( loadFunc );

		return snd;
	}

	void AudioSystem::RequestNewSounds( std::vector<AssetID> Ids, std::vector<UUID> PlayerIds, std::function<void(Ref<Sound>)>&& rVistor )
	{
		// Copy vectors because they might be destroyed by the time the audio thread gets the this function.
		auto loadFunc = [copyIds = Ids, copyPlayerIds = PlayerIds, rVistor, this]()
			{
				size_t soundIndex = 0;
				for( const AssetID& rAssetID : copyIds )
				{
					Ref<Sound> snd = RequestNewSound( rAssetID, copyPlayerIds[ soundIndex ] );

					if( rVistor )
						rVistor( snd );

					++soundIndex;
				}
			};

		m_AudioThread->IsCurrentThread() ? loadFunc() : m_AudioThread->Queue( loadFunc );
	}

	void AudioSystem::ReportSoundCompleted( UUID UniquePlayerID )
	{
		const auto Itr = m_AliveSounds.find( UniquePlayerID );

		if( Itr != m_AliveSounds.end() ) 
		{
			auto& rSnd = ( Itr )->second;
			rSnd->Stop();
			rSnd->OnSoundCompleted();

			m_AliveSounds.erase( Itr );
		}
	}

	void AudioSystem::ReportSoundPlayingIfNeeded( UUID UniquePlayerID )
	{
		const auto Itr = m_AliveSounds.find( UniquePlayerID );
		if( Itr == m_AliveSounds.end() )
		{
			m_AliveSounds[ UniquePlayerID ] = m_LoadedSounds[ UniquePlayerID ];
		}
	}

	void AudioSystem::Suspend()
	{
		// Stop and unload any alive sounds
		for( auto& [id, sound] : m_AliveSounds )
		{
			sound->Stop();
		}
	
		MA_CHECK( ma_device_stop( m_pDevice ) );
	}

	void AudioSystem::Resume()
	{
		MA_CHECK( ma_device_start( m_pDevice ) );

		for( auto& [id, sound] : m_AliveSounds )
		{
			sound->Play( 0 );
		}
	}

	void AudioSystem::SetPrimaryListenerPos( const glm::vec3& rPos )
	{
		ma_engine_listener_set_position( m_pEngine, 0, rPos.x, rPos.y, rPos.z );
	}

	void AudioSystem::SetPrimaryListenerDirection( const glm::vec3& rDir )
	{
		ma_engine_listener_set_direction( m_pEngine, 0, rDir.x, rDir.y, rDir.z );
	}

	void AudioSystem::DestroySoundsInSet( uint8_t set )
	{
		std::erase_if( m_AliveSounds, 
			[ set ](auto& kv) -> bool
		{
			if( kv.second->GetSet() == set ) 
			{
				kv.second->Unload();
				return true;
			}

			return false;
		} );

		std::erase_if( m_LoadedSounds,
			[ set ]( const auto& kv ) -> bool
		{
			return kv.second->GetSet() == set;
		} );
	}

	void AudioSystem::StopSoundInSet( uint8_t set )
	{
		for( auto& [id, sound] : m_AliveSounds )
		{
			if( sound->GetSet() == set )
				sound->Stop();
		}
	}

	void AudioSystem::StopAndResetSound( UUID UniquePlayerID )
	{
		const auto Itr = std::find_if( m_LoadedSounds.begin(), m_LoadedSounds.end(), 
			[UniquePlayerID]( const auto& kv )
			{
				return kv.first == UniquePlayerID;
			} );

		if( Itr != m_LoadedSounds.end() )
		{
			auto& rSound = ( Itr->second );

			rSound->Stop();
			rSound->Reset();
		}
	}

	void AudioSystem::StopSound( UUID UniquePlayerID )
	{
		const auto Itr = std::find_if( m_LoadedSounds.begin(), m_LoadedSounds.end(),
			[ UniquePlayerID ]( const auto& kv )
		{
			return kv.first == UniquePlayerID;
		} );

		if( Itr != m_LoadedSounds.end() )
		{
			auto& rSound = ( Itr->second );

			rSound->Stop();
		}
	}

	bool AudioSystem::DoesSoundExist( UUID UniquePlayerID )
	{
		return m_LoadedSounds.contains( UniquePlayerID ) || m_AliveSounds.contains( UniquePlayerID );
	}

	void AudioSystem::UnloadSound( Ref<SoundBase> sound )
	{
		if( !sound ) return;

		auto& playerID = sound->m_PlayerID;

		sound->Stop();
		sound->Unload();

		m_LoadedSounds.erase( playerID );

		const auto Itr = std::find_if( m_AliveSounds.begin(), m_AliveSounds.end(), 
			[playerID](const auto& kv)
			{
				return kv.first == playerID;
			} );

		if( Itr != m_AliveSounds.end() )
			m_AliveSounds.erase( Itr );

		// TODO: Right now MarkForDestroy does not really do anything
		//		 sound will be destroyed at the end of this scope (ref count should be 1).
		sound->MarkForDestroy();
	}

	void AudioSystem::UnloadSound( UUID UniquePlayerID )
	{
		const auto Itr = std::find_if( m_LoadedSounds.begin(), m_LoadedSounds.end(),
			[UniquePlayerID]( const auto& kv )
			{
				return kv.first == UniquePlayerID;
			} );

		if( Itr != m_LoadedSounds.end() )
		{
			auto& rSound = ( Itr->second );

			UnloadSound( rSound );
		}
	}

	void AudioSystem::StartSoundGroups()
	{
		/*
		if( m_MasterSoundGroup )
			m_MasterSoundGroup->Start();

		for( auto& rSoundGroup : Project::GetActiveProject()->GetSoundGroups() )
		{
			rSoundGroup->Start();
		}
		*/
	}

	void AudioSystem::StopSoundGroups()
	{
		/*
		for( auto& rSoundGroup : Project::GetActiveProject()->GetSoundGroups() )
		{
			rSoundGroup->Stop();
		}

		if( m_MasterSoundGroup )
			m_MasterSoundGroup->Stop();
		*/
	}

	SoundDecodedInformation AudioSystem::DecodeSound( const Ref<SoundSpecification>& rSpec )
	{
		SoundDecodedInformation decodedInformation{};
		
		ma_decoder decoder;
		ma_decoder_config config{};
		
		config = ma_decoder_config_init( ma_format_f32, 2, m_pEngine->sampleRate );

		MA_CHECK( ma_decoder_init_file( rSpec->SoundSourcePath.string().data(), &config, &decoder ) );

		ma_uint64 frames = 0;
		MA_CHECK( ma_decoder_get_length_in_pcm_frames( &decoder, &frames ) );

		const uint64_t bpf = ma_get_bytes_per_frame( decoder.outputFormat, decoder.outputChannels );
		const size_t bufferSize = frames * bpf;

		decodedInformation.PCMFrameCount = ( uint64_t ) frames;
		decodedInformation.BytesPerFrame = bpf;
		decodedInformation.Channels = decoder.outputChannels;
		decodedInformation.SampleRate = decoder.outputSampleRate;
		decodedInformation.Format = ( int ) decoder.outputFormat;

		Buffer TemporaryBuffer;
		TemporaryBuffer.Allocate( bufferSize );
		TemporaryBuffer.Zero_Memory();

		ma_uint64 totalFrameRead = 0;
		MA_CHECK( ma_decoder_read_pcm_frames( &decoder, TemporaryBuffer.Data, frames, &totalFrameRead ) );

		SAT_CORE_ASSERT( totalFrameRead == frames, "Audio decoder did not read the whole file/buffer!" );
	
		MA_CHECK( ma_decoder_uninit( &decoder ) );

		decodedInformation.PCMFrames = Buffer::Copy( TemporaryBuffer.Data, TemporaryBuffer.Size );
		TemporaryBuffer.Free();

		return decodedInformation;
	}

	Ref<Sound> AudioSystem::FindSound( UUID UniquePlayerID )
	{
		const auto Itr = std::find_if( m_AliveSounds.begin(), m_AliveSounds.end(),
			[UniquePlayerID]( const auto& kv )
			{
				return kv.first == UniquePlayerID;
			} );

		if( Itr != m_AliveSounds.end() ) 
			return Itr->second;

		// Check if the sound is in the loaded map
		const auto LoadedItr = std::find_if( m_LoadedSounds.begin(), m_LoadedSounds.end(),
			[UniquePlayerID]( const auto& kv )
			{
				return kv.first == UniquePlayerID;
			} );

		if( LoadedItr != m_LoadedSounds.end() )
			return LoadedItr->second;

		return nullptr;
	}

}
