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

#include "SingletonStorage.h"
#include "Saturn/Core/Thread.h"

#include "AudioCore.h"
#include "Sound.h"
#include "SoundGroup.h"
#include "GraphSound.h"

namespace Saturn {

	class AudioThread : public Thread
	{
	public:
		AudioThread();
		virtual ~AudioThread();

	public:
		virtual void Start() override;
		virtual void RequestJoin() override;

	private:
		void ThreadRun();

	private:
		std::atomic_bool m_PendingTerminate{ false };
	};

	class AudioSystem
	{
	public:
		SAT_SINGLETON_LAZY( AudioSystem )

	public:
		AudioSystem();
		~AudioSystem();

		void Terminate();
		void WaitForInit();

		// Loads a new Sound2D to be played
		// By default the sound will automatically play upon loading.
		// Set index == 1
		// WARNING:
		//  This function uses the Audio Thread
		//  This function will return the sound however, it may not be loaded as soon as it returns!
		//  Use WaitUntilLoaded for safety.
		Ref<Sound> RequestNewSound( AssetID ID, UUID UniquePlayerID = Saturn::UUID(), bool PlayNow = true, Ref<SoundGroup> soundGroup = nullptr );

		//
		// Play a new Sound2D and forget about it.
		// 
		// @param rAssetName -- the name of the SoundSpec asset to be played.
		//
		void FireAndForget( const std::string& rAssetName );

		//
		// Play a new Sound2D and forget about it.
		// 
		// @param id -- the AssetID of the SoundSpec asset to be played.
		//
		void FireAndForget( AssetID id );

		// Loads a new spatialised sound to be played.
		// Set index == 1
		// WARNING:
		//  This function uses the Audio Thread
		//  This function will return the sound however, it may not be loaded as soon as it returns!
		//  Use WaitUntilLoaded for safety.
		Ref<Sound> PlaySoundAtLocation( AssetID ID, UUID UniquePlayerID, const glm::vec3& rPos, bool PlayNow = true, Ref<SoundGroup> soundGroup = nullptr );
	
		Ref<GraphSound> PlayGraphSound( AssetID ID, UUID UniquePlayerID = Saturn::UUID(), bool spatialisation = false, bool PlayNow = true );

		// Load a sound in set index 0
		Ref<Sound> RequestPreviewSound( AssetID ID, UUID UniquePlayerID = Saturn::UUID(), bool PlayNow = true, Ref<SoundGroup> soundGroup = nullptr );

		void RequestNewSounds( std::vector<AssetID> Ids, std::vector<UUID> PlayerIds, std::function<void(Ref<Sound>)>&& rVistor );

		void ReportSoundCompleted( UUID UniquePlayerID );
		void ReportSoundPlayingIfNeeded( UUID UniquePlayerID );

		// Suspend ALL sounds and pause the Audio device interface.
		void Suspend();

		// Resume ALL sounds and resume the Audio device interface.
		void Resume();

		void SetPrimaryListenerPos( const glm::vec3& rPos );
		void SetPrimaryListenerDirection( const glm::vec3& rDir );

		void DestroySoundsInSet( uint8_t set );
		void StopSoundInSet( uint8_t set );
		void StopAndResetSound( UUID UniquePlayerID );
		void StopSound( UUID UniquePlayerID );

		[[nodiscard]] bool DoesSoundExist( UUID UniquePlayerID );

		void UnloadSound( Ref<SoundBase> sound );
		void UnloadSound( UUID UniquePlayerID );

		void StartSoundGroups();
		void StopSoundGroups();

		ma_engine* GetAudioEngine() const { return m_pEngine; }
		Ref<SoundGroup>& GetMasterSoundGroup() { return m_MasterSoundGroup; }

		SoundDecodedInformation DecodeSound( const Ref<SoundSpecification>& rSpec );

		Ref<Sound> FindSound( UUID UniquePlayerID );

		Ref<AudioThread> GetThread() { return m_AudioThread; }

	private:
		void Initialise();
		void PlaySound( Ref<Sound> rSoundAsset );

	private:
		Ref<AudioThread> m_AudioThread;
		Ref<SoundGroup> m_MasterSoundGroup;

		// Currently alive sounds (i.e. sounds that are playing)
		std::unordered_map<UUID, Ref<SoundBase>> m_AliveSounds;
		// A list of every loaded sound in memory.
		std::unordered_map<UUID, Ref<SoundBase>> m_LoadedSounds;

	private:
		ma_engine*  m_pEngine;
		ma_context* m_pContext;
		ma_device*  m_pDevice;

		std::atomic<bool> m_Initialised = false;
	};
}
